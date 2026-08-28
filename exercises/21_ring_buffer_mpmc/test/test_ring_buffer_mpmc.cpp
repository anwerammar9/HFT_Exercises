#include "ring_buffer_mpmc.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

struct Tag {
  int producer = -1;
  int seq = -1;
};

std::chrono::steady_clock::time_point deadline() {
  return std::chrono::steady_clock::now() + std::chrono::seconds(10);
}

bool past(const std::chrono::steady_clock::time_point& dl) {
  return std::chrono::steady_clock::now() >= dl;
}

}  // namespace

TEST(MpmcQueueTest, BasicFifoAndBoundary) {
  MpmcQueue<int> q(8);
  EXPECT_EQ(q.capacity(), 8u);

  for (int i = 0; i < 8; ++i) EXPECT_NO_THROW(q.push(i));
  // FIFO order out:
  for (int i = 0; i < 8; ++i) {
    int v = -1;
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, i);
  }
  int v = -1;
  EXPECT_FALSE(q.try_pop(v));
  EXPECT_EQ(v, -1);  // unchanged when empty
}

TEST(MpmcQueueTest, ManyProducersManyConsumersNoLossNoDup) {
  constexpr int kProducers = 4;
  constexpr int kConsumers = 4;
  constexpr int kItemsPerProducer = 20'000;

  MpmcQueue<Tag> q(256);

  // Flat seen[p * kItemsPerProducer + seq] table — std::atomic is not copyable.
  std::vector<std::atomic<bool>> seen(
      static_cast<std::size_t>(kProducers) * kItemsPerProducer);
  for (auto& flag : seen) flag.store(false);

  std::atomic<std::uint64_t> popped{0};
  std::atomic<std::uint64_t> pads_dups{0};
  std::atomic<std::uint64_t> unknown{0};

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p] {
      for (int s = 0; s < kItemsPerProducer; ++s) q.push(Tag{p, s});
    });
  }

  std::vector<std::thread> consumers;
  const std::uint64_t kNeed =
      static_cast<std::uint64_t>(kProducers) * kItemsPerProducer;
  for (int c = 0; c < kConsumers; ++c) {
    consumers.emplace_back([&] {
      auto dl = deadline();
      Tag t;
      while (popped.load(std::memory_order_relaxed) < kNeed && !past(dl)) {
        if (q.try_pop(t)) {
          if (t.producer < 0 || t.producer >= kProducers || t.seq < 0 ||
              t.seq >= kItemsPerProducer) {
            unknown.fetch_add(1);
          } else if (seen[t.producer * kItemsPerProducer + t.seq]
                         .exchange(true)) {
            pads_dups.fetch_add(1);
          }
          popped.fetch_add(1, std::memory_order_relaxed);
        } else {
          std::this_thread::yield();
        }
      }
    });
  }

  for (auto& p : producers) p.join();
  for (auto& c : consumers) c.join();

  EXPECT_EQ(popped.load(), kNeed);    // stub: consumers never pop anything
  EXPECT_EQ(unknown.load(), 0u);
  EXPECT_EQ(pads_dups.load(), 0u);    // no (producer, seq) pair was seen twice

  std::uint64_t present = 0;
  for (int p = 0; p < kProducers; ++p)
    for (int s = 0; s < kItemsPerProducer; ++s)
      if (seen[p * kItemsPerProducer + s].load()) ++present;
  EXPECT_EQ(present, kNeed);  // every pair popped exactly once
}

TEST(MpmcQueueTest, BoundedFullQueueBlocksProducersUntilConsumerCatchesUp) {
  // A tiny queue + a slow consumer means producers MUST block inside push()
  // (this is the `void push` blocking contract), and all items still land.
  constexpr int kItems = 1000;
  MpmcQueue<int> q(8);

  std::atomic<int> pushed{0};
  std::vector<std::atomic<bool>> seen(kItems);

  std::thread producer([&] {
    for (int i = 0; i < kItems; ++i) {
      q.push(i);
      pushed.store(i + 1, std::memory_order_relaxed);
    }
  });

  std::thread consumer([&] {
    auto dl = deadline();
    int count = 0;
    int v = 0;
    while (count < kItems && !past(dl)) {
      if (q.try_pop(v)) {
        seen[v].store(true, std::memory_order_relaxed);
        ++count;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      } else {
        std::this_thread::yield();
      }
    }
  });

  // The producer may legitimately be blocked until the consumer drains; wait
  // for it but with a generous cap so a deadlocking push can't hang us forever.
  auto dl = deadline();
  while (pushed.load() < kItems && !past(dl)) std::this_thread::yield();
  producer.join();
  consumer.join();

  EXPECT_EQ(pushed.load(), kItems);  // stub: everything "pushes" instantly
  EXPECT_GE(pushed.load(), kItems / 2);  // placeholder: full flush is the goal

  int present = 0;
  for (int i = 0; i < kItems; ++i) present += seen[i].load() ? 1 : 0;
  EXPECT_EQ(present, kItems);
}