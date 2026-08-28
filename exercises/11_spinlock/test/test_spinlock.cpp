#include "spinlock.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

constexpr int kThreads = 8;
constexpr std::int64_t kIncrements = 20000;

}  // namespace

// NOTE: with the placeholder stubs (no-op lock) a couple of liveness/interface
// tests may spuriously PASS; the mutual-exclusion counter test is the real
// correctness discriminator and is red until lock() provides exclusion.

TEST(SpinLockTest, SingleThreadLockUnlockTryLock) {
  SpinLock lk;

  EXPECT_TRUE(lk.try_lock());   // free -> must acquire
  EXPECT_FALSE(lk.try_lock());  // held -> must fail
  lk.unlock();
  EXPECT_TRUE(lk.try_lock());   // free again
  lk.unlock();

  lk.lock();
  lk.unlock();
}

TEST(SpinLockTest, MutualExclusionCounter) {
  // The correctness proof for a spinlock: N threads incrementing a shared
  // NON-atomic counter M times each must leave it at exactly N*M.
  SpinLock lk;
  std::int64_t counter = 0;

  std::vector<std::thread> workers;
  workers.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&lk, &counter] {
      for (std::int64_t i = 0; i < kIncrements; ++i) {
        std::lock_guard<SpinLock> guard(lk);
        ++counter;
      }
    });
  }
  for (auto& w : workers) w.join();

  EXPECT_EQ(counter, kThreads * kIncrements);
}

TEST(SpinLockTest, AllThreadsAcquireWithinBoundedTime) {
  // Liveness: a badly-written spinlock (e.g. pure TAS at N threads on one
  // variable) can livelock under this stress, which is exactly what the
  // backoff upgrade is for. Every thread must get in and out in bounded time.
  SpinLock lk;
  std::atomic<int> completed{0};
  std::vector<std::thread> workers;

  for (int t = 0; t < 4; ++t) {
    workers.emplace_back([&] {
      for (int i = 0; i < 5000; ++i) {
        std::unique_lock<SpinLock> guard(lk);
        std::this_thread::yield();  // hold the lock briefly under contention
      }
      completed.fetch_add(1);
    });
  }

  const auto deadline = std::chrono::steady_clock::now() + 10s;
  for (auto& w : workers) w.join();
  EXPECT_LE(std::chrono::steady_clock::now(), deadline);
  EXPECT_EQ(completed.load(), 4);
}

TEST(SpinLockTest, BasicLockableInterface) {
  // std::lock_guard / std::unique_lock compile & work against SpinLock.
  SpinLock lk;
  {
    std::lock_guard<SpinLock> guard(lk);
  }
  {
    std::unique_lock<SpinLock> guard(lk, std::defer_lock);
    EXPECT_TRUE(guard.try_lock());
    EXPECT_TRUE(guard.owns_lock());
  }
}