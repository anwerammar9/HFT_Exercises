#include "counting_semaphore.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

constexpr int kHolders = 4;  // semaphore budget for the concurrency test

}  // namespace

// NOTE: a couple of tests below can spuriously PASS against the placeholder
// stubs (e.g. a no-op acquire() looks "free" in single-threaded checks). The
// blocking test (AcquireBlocksUntilRelease) and the budget test
// (AtMostLimitThreadsHoldSimultaneously) are the RED discriminators: against a
// no-op stub they fail fast, they never hang.

TEST(SemaphoreTest, StartsWithGivenTokens) {
  Semaphore s(3);
  EXPECT_EQ(s.count(), 3u);

  s.acquire();
  s.acquire();
  EXPECT_EQ(s.count(), 1u);
}

TEST(SemaphoreTest, EmptySemaphoreTryAcquireFailsWithoutConsuming) {
  Semaphore s(0);
  EXPECT_FALSE(s.try_acquire());
  EXPECT_EQ(s.count(), 0u);  // a failed try_acquire must NOT consume
}

TEST(SemaphoreTest, ReleaseAddsBackAToken) {
  Semaphore s(0);
  s.release();
  EXPECT_EQ(s.count(), 1u);
  EXPECT_TRUE(s.try_acquire());
  EXPECT_EQ(s.count(), 0u);
}

TEST(SemaphoreTest, ReleaseAccumulates) {
  Semaphore s(0);
  for (int i = 0; i < 5; ++i) s.release();
  EXPECT_EQ(s.count(), 5u);
  for (int i = 0; i < 5; ++i) EXPECT_TRUE(s.try_acquire());
  EXPECT_EQ(s.count(), 0u);
}

TEST(SemaphoreTest, AcquireBlocksUntilRelease) {
  Semaphore s(0);
  std::atomic<bool> acquired{false};
  std::thread t([&] {
    s.acquire();
    acquired.store(true);
  });

  // Give the thread time to reach the blocking wait; it must still be blocked.
  std::this_thread::sleep_for(20ms);
  EXPECT_FALSE(acquired.load()) << "acquire() must block on an empty semaphore";

  s.release();
  t.join();
  EXPECT_TRUE(acquired.load());
}

TEST(SemaphoreTest, TryAcquireHandsOutBudgetExactlyOnce) {
  Semaphore s(2);
  int handed = 0;
  while (s.try_acquire()) ++handed;
  EXPECT_EQ(handed, 2);
  EXPECT_EQ(s.count(), 0u);
}

TEST(SemaphoreTest, AtMostLimitThreadsHoldSimultaneously) {
  // The discriminator: N tokens must bound the number of threads inside the
  // guarded section, no matter how many ask. Against a no-op stub, all 16
  // threads enter at once and the peak exceeds the budget -> red.
  Semaphore s(kHolders);
  std::atomic<int> inside{0};
  std::atomic<int> peak{0};
  std::atomic<bool> over_budget{false};

  std::vector<std::thread> workers;
  for (int t = 0; t < 4 * kHolders; ++t) {
    workers.emplace_back([&] {
      s.acquire();
      const int now = inside.fetch_add(1) + 1;
      std::this_thread::sleep_for(1ms);  // hold to widen the observation window
      if (now > kHolders) over_budget.store(true);
      int pk = peak.load();
      while (pk < now && !peak.compare_exchange_weak(pk, now)) {}
      inside.fetch_sub(1);
      s.release();
    });
  }
  for (auto& w : workers) w.join();

  EXPECT_FALSE(over_budget.load());
  EXPECT_LE(peak.load(), kHolders);
}