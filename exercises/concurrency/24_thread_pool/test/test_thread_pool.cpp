#include "thread_pool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

TEST(ThreadPoolTest, TasksExecuteExactlyOnceAndResultsRetrievable) {
  ThreadPool pool(4);
  constexpr int kTasks = 64;

  std::vector<std::future<int>> futures;
  futures.reserve(kTasks);
  for (int i = 0; i < kTasks; ++i) {
    futures.push_back(pool.submit([i] { return i * i; }));
  }

  std::int64_t sum = 0;
  for (auto& f : futures) sum += f.get();
  EXPECT_EQ(sum, kTasks * (kTasks - 1) * (2 * kTasks - 1) / 6);
}

TEST(ThreadPoolTest, ExceptionsPropagateThroughFuture) {
  ThreadPool pool(2);
  auto f = pool.submit([] { throw std::runtime_error("boom"); return 0; });
  EXPECT_THROW(f.get(), std::runtime_error);
}

TEST(ThreadPoolTest, PoolRunsTasksConcurrently) {
  ThreadPool pool(4);

  std::atomic<int> active{0};
  std::atomic<int> peak{0};

  std::vector<std::future<void>> futures;
  for (int i = 0; i < 8; ++i) {
    futures.push_back(pool.submit([&] {
      const int cur = active.fetch_add(1) + 1;
      for (int p = peak.load(); cur > p && !peak.compare_exchange_weak(p, cur);) {}
      std::this_thread::sleep_for(50ms);  // widen the window for the peak to be seen
      active.fetch_sub(1);
    }));
  }
  for (auto& f : futures) f.get();

  EXPECT_GT(peak.load(), 1);  // with 4 workers, at least two overlapped
}

TEST(ThreadPoolTest, DestructorDrainsEverySubmittedTask) {
  std::atomic<int> ran{0};

  {
    std::unique_ptr<ThreadPool> pool =
        std::make_unique<ThreadPool>(3);
    for (int i = 0; i < 200; ++i) {
      pool->submit([&ran] { ran.fetch_add(1); });
    }
    pool.reset();  // shutdown inside ~ThreadPool must run all 200 tasks
  }

  EXPECT_EQ(ran.load(), 200);  // no task dropped before destruction returns
}

TEST(ThreadPoolTest, SubmitAfterShutdownIsRejected) {
  ThreadPool pool(2);
  pool.shutdown();

  // Contract: submit() after shutdown throws std::logic_error.
  // NOTE: with today's stub this EXPECT_THROW passes spuriously, because the
  // stub enqueue throws "not implemented" regardless of shutdown state. It
  // becomes meaningful once the real queue exists.
  EXPECT_THROW(pool.submit([] {}), std::logic_error);
}

TEST(ThreadPoolTest, ShutdownWaitsForInflightAndIsIdempotent) {
  ThreadPool pool(2);

  std::atomic<int> completed{0};
  auto f = pool.submit([&] {
    std::this_thread::sleep_for(50ms);
    completed.fetch_add(1);
    return 42;
  });

  pool.shutdown();   // must wait for the in-flight task
  EXPECT_EQ(f.get(), 42);
  pool.shutdown();   // idempotent: must not throw / deadlock
  EXPECT_EQ(completed.load(), 1);
}