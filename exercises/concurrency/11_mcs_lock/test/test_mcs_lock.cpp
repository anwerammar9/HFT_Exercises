#include "mcs_lock.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

constexpr int kThreads = 8;
constexpr std::int64_t kIncrements = 20000;

}  // namespace

// NOTE: the no-op stub lock lets every thread into the critical section at
// once, so the counter and peak tests are RED (the discriminators); the
// single-threaded and liveness tests can pass spuriously against the stub.

TEST(McsLockTest, SingleThreadLockUnlock) {
  McsLock lk;
  McsLock::Waiter w;

  lk.lock(w);
  lk.unlock(w);

  // Same node reused for a second round — the expected pattern.
  lk.lock(w);
  lk.unlock(w);
}

TEST(McsLockTest, MutualExclusionCounter) {
  McsLock lk;
  std::int64_t counter = 0;
  std::vector<McsLock::Waiter> waiters(kThreads);

  std::vector<std::thread> workers;
  workers.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&, t] {
      for (std::int64_t i = 0; i < kIncrements; ++i) {
        lk.lock(waiters[t]);
        ++counter;  // non-atomic: correctness proves mutual exclusion
        lk.unlock(waiters[t]);
      }
    });
  }
  for (auto& w : workers) w.join();

  EXPECT_EQ(counter, kThreads * kIncrements);
}

TEST(McsLockTest, OnlyOneHolderAtATime) {
  McsLock lk;
  std::atomic<int> inside{0};
  std::atomic<int> peak{0};
  std::atomic<bool> over{false};

  std::vector<McsLock::Waiter> waiters(kThreads);
  std::vector<std::thread> workers;
  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&, t] {
      for (int i = 0; i < 2000; ++i) {
        lk.lock(waiters[t]);
        const int now = inside.fetch_add(1) + 1;
        if (now > 1) over.store(true);
        int pk = peak.load();
        while (pk < now && !peak.compare_exchange_weak(pk, now)) {}
        std::this_thread::yield();  // hold a moment to expose a broken lock
        inside.fetch_sub(1);
        lk.unlock(waiters[t]);
      }
    });
  }
  for (auto& w : workers) w.join();

  EXPECT_FALSE(over.load());
  EXPECT_EQ(peak.load(), 1);
}

TEST(McsLockTest, AllThreadsMakeProgressUnderContention) {
  // Liveness: every thread must acquire and release a bounded number of times.
  const auto start = std::chrono::steady_clock::now();
  McsLock lk;
  std::atomic<int> completed{0};
  std::vector<McsLock::Waiter> waiters(kThreads);
  std::vector<std::thread> workers;
  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&, t] {
      for (int i = 0; i < 5000; ++i) {
        lk.lock(waiters[t]);
        lk.unlock(waiters[t]);
      }
      completed.fetch_add(1);
    });
  }
  for (auto& w : workers) w.join();

  const auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_LT(elapsed, 10s) << "MCS lock must not livelock under contention";
  EXPECT_EQ(completed.load(), kThreads);
}

TEST(McsLockTest, SingleNodeReusedAcrossManyRounds) {
  McsLock lk;
  McsLock::Waiter w;
  constexpr std::int64_t kRounds = 100000;
  for (std::int64_t i = 0; i < kRounds; ++i) {
    lk.lock(w);
    lk.unlock(w);
  }
}

TEST(McsLockTest, TwoThreadsPingPongBothComplete) {
  McsLock lk;
  constexpr std::int64_t kRounds = 20000;
  std::atomic<std::int64_t> count_a{0};
  std::atomic<std::int64_t> count_b{0};
  McsLock::Waiter wa;
  McsLock::Waiter wb;

  std::thread a([&] {
    for (std::int64_t i = 0; i < kRounds; ++i) {
      lk.lock(wa);
      count_a.fetch_add(1);
      lk.unlock(wa);
    }
  });
  std::thread b([&] {
    for (std::int64_t i = 0; i < kRounds; ++i) {
      lk.lock(wb);
      count_b.fetch_add(1);
      lk.unlock(wb);
    }
  });
  a.join();
  b.join();

  EXPECT_EQ(count_a.load(), kRounds);
  EXPECT_EQ(count_b.load(), kRounds);
}