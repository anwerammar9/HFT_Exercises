#include "ring_buffer_spsc.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

namespace {

constexpr std::uint64_t kTotal = 5'000'000;

std::chrono::steady_clock::time_point deadline() {
  // A correct SPSC pair drains 5M items in well under a second. Give an
  // implementation a generous budget; stubs hit the deadline and fail red.
  return std::chrono::steady_clock::now() + std::chrono::seconds(10);
}

bool past(const std::chrono::steady_clock::time_point& dl) {
  return std::chrono::steady_clock::now() >= dl;
}

}  // namespace

TEST(SpscRingBufferTest, FifoWithinCapacityWithOneFreeSlot) {
  using Buf = SpscRingBuffer<std::uint64_t, 1024>;
  Buf buf;

  EXPECT_TRUE(buf.empty());
  EXPECT_FALSE(buf.full());

  // Fill to usable capacity (N-1); the (N-1)th slot must be accepted...
  for (std::uint64_t i = 0; i < Buf::kCapacity; ++i) {
    EXPECT_TRUE(buf.try_push(i)) << "push " << i << " must succeed";
  }
  EXPECT_EQ(buf.size(), Buf::kCapacity);
  EXPECT_TRUE(buf.full());
  EXPECT_FALSE(buf.empty());

  // ...and the Nth must be rejected (buffer is full).
  EXPECT_FALSE(buf.try_push(999));

  // Drain in FIFO order.
  std::uint64_t v = 0;
  for (std::uint64_t i = 0; i < Buf::kCapacity; ++i) {
    EXPECT_TRUE(buf.try_pop(v));
    EXPECT_EQ(v, i);
  }
  EXPECT_TRUE(buf.empty());
  EXPECT_FALSE(buf.full());
  EXPECT_EQ(buf.size(), 0u);

  // And now pop fails again.
  EXPECT_FALSE(buf.try_pop(v));
}

TEST(SpscRingBufferTest, PushWhenFullFails) {
  SpscRingBuffer<std::uint64_t, 8> buf;  // usable capacity 7
  for (std::uint64_t i = 0; i < 7; ++i) EXPECT_TRUE(buf.try_push(i));
  EXPECT_TRUE(buf.full());
  EXPECT_FALSE(buf.try_push(7));
}

TEST(SpscRingBufferTest, PopWhenEmptyFails) {
  SpscRingBuffer<std::uint64_t, 8> buf;
  std::uint64_t v = 42;
  EXPECT_FALSE(buf.try_pop(v));
  EXPECT_EQ(v, 42);  // out-param untouched when pop fails
}

TEST(SpscRingBufferTest, ProducerConsumerDeliversEveryItemInOrder) {
  using Buf = SpscRingBuffer<std::uint64_t, 1024>;
  Buf buf;

  std::atomic<std::uint64_t> pushed{0};
  std::atomic<std::uint64_t> consumed{0};
  std::atomic<std::uint64_t> mismatches{0};

  std::thread producer([&] {
    auto dl = deadline();
    std::uint64_t i = 0;
    while (i < kTotal && !past(dl)) {
      if (buf.try_push(i)) {
        ++i;
        pushed.store(i, std::memory_order_relaxed);
      } else {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&] {
    auto dl = deadline();
    std::uint64_t next = 0;
    std::uint64_t v = 0;
    while (next < kTotal && !past(dl)) {
      if (buf.try_pop(v)) {
        if (v != next) mismatches.fetch_add(1);
        ++next;
        consumed.store(next, std::memory_order_relaxed);
      } else {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(pushed.load(), kTotal);    // stub: producer never gets a slot
  EXPECT_EQ(consumed.load(), kTotal);  // stub: consumer never gets an item
  EXPECT_EQ(mismatches.load(), 0u);    // every item arrived exactly in order
}