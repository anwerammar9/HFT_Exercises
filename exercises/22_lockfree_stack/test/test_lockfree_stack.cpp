#include "lockfree_stack.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
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

TEST(LockFreeStackTest, SingleThreadLifoOrdering) {
  LockFreeStack<int> s;
  s.push(1);
  s.push(2);
  s.push(3);

  int v = 0;
  EXPECT_TRUE(s.try_pop(v));
  EXPECT_EQ(v, 3);
  EXPECT_TRUE(s.try_pop(v));
  EXPECT_EQ(v, 2);
  EXPECT_TRUE(s.try_pop(v));
  EXPECT_EQ(v, 1);
}

TEST(LockFreeStackTest, EmptyPopIsSafeNoOp) {
  LockFreeStack<int> s;
  int v = 42;
  EXPECT_FALSE(s.try_pop(v));
  EXPECT_EQ(v, 42);

  s.push(7);
  EXPECT_TRUE(s.try_pop(v));
  EXPECT_EQ(v, 7);
  EXPECT_FALSE(s.try_pop(v));  // and it's empty again
}

TEST(LockFreeStackTest, ConcurrentPushPopNoLossNoCrash) {
  constexpr int kProducers = 4;
  constexpr int kConsumers = 4;
  constexpr int kItemsPerProducer = 20'000;

  const std::int64_t kNeed = kProducers * kItemsPerProducer;

  LockFreeStack<Tag> s;

  std::atomic<std::int64_t> pushed{0};
  std::atomic<std::int64_t> popped_valid{0};
  std::atomic<std::uint64_t> dups{0};
  std::atomic<std::uint64_t> bad{0};
  std::vector<std::atomic<bool>> seen(
      static_cast<std::size_t>(kProducers) * kItemsPerProducer);
  for (auto& flag : seen) flag.store(false);

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p] {
      for (int i = 0; i < kItemsPerProducer; ++i) {
        s.push(Tag{p, i});
        pushed.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  std::vector<std::thread> consumers;
  for (int c = 0; c < kConsumers; ++c) {
    consumers.emplace_back([&] {
      auto dl = deadline();
      Tag t;
      while (popped_valid.load(std::memory_order_relaxed) < kNeed &&
             !past(dl)) {
        if (s.try_pop(t)) {
          if (t.producer < 0 || t.producer >= kProducers || t.seq < 0 ||
              t.seq >= kItemsPerProducer) {
            bad.fetch_add(1);
          } else if (seen[t.producer * kItemsPerProducer + t.seq]
                         .exchange(true)) {
            dups.fetch_add(1);  // ABA bug / re-push of a live node would show up
          }
          popped_valid.fetch_add(1, std::memory_order_relaxed);
        } else {
          std::this_thread::yield();
        }
      }
    });
  }

  for (auto& p : producers) p.join();
  for (auto& c : consumers) c.join();

  EXPECT_EQ(pushed.load(), kNeed);
  EXPECT_EQ(popped_valid.load(), kNeed);  // stub: nothing is ever popped
  EXPECT_EQ(dups.load(), 0u);
  EXPECT_EQ(bad.load(), 0u);
  EXPECT_EQ(pushed.load(), popped_valid.load());  // nothing lost, nothing added
}