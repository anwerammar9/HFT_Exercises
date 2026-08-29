#include "serial_executor.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// NOTE: with the placeholder stubs the serial-executor tests fail RED in one of
// two ways — submit() itself throws (stub enqueue throws "not implemented"),
// or an expectation like the ordering/concurrency check fails. They never
// hang: no test calls future.get() before drain(), and submit() throws before
// returning a future on the stub. SubmitAfterShutdownIsRejected can spuriously
// PASS (the stub throws regardless of shutdown state), exactly like 14.

TEST(SerialExecutorTest, TasksExecuteExactlyOnceInSubmissionOrder) {
  SerialExecutor ex;
  std::vector<int> log;
  log.reserve(100);
  std::vector<std::future<void>> futures;

  for (int i = 0; i < 100; ++i) {
    futures.push_back(ex.submit([&log, i] { log.push_back(i); }));
  }
  ex.drain();
  for (auto& f : futures) f.get();  // no exceptions, all ran

  ASSERT_EQ(log.size(), 100u);
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(log[i], i) << "FIFO order violated for submission index " << i;
  }
}

TEST(SerialExecutorTest, SubmitReturnsResultsFromArgs) {
  SerialExecutor ex;
  auto f1 = ex.submit([](int a, int b) { return a + b; }, 2, 3);
  auto f2 = ex.submit([](double x) { return x * 0.5; }, 10.0);
  auto f3 = ex.submit([] { return std::string("done"); });

  EXPECT_EQ(f1.get(), 5);
  EXPECT_DOUBLE_EQ(f2.get(), 5.0);
  EXPECT_EQ(f3.get(), "done");
}

TEST(SerialExecutorTest, ExactlyOneTaskRunsAtATime) {
  // The serial discriminator: with 32 tasks submitted in a burst, the number
  // of tasks running CONCURRENTLY must never exceed 1.
  SerialExecutor ex;
  std::atomic<int> inside{0};
  std::atomic<int> peak{0};

  std::vector<std::future<void>> futures;
  for (int i = 0; i < 32; ++i) {
    futures.push_back(ex.submit([&] {
      const int now = inside.fetch_add(1) + 1;
      std::this_thread::sleep_for(2ms);  // widen the overlap window
      int p = peak.load();
      while (p < now && !peak.compare_exchange_weak(p, now)) {}
      inside.fetch_sub(1);
    }));
  }
  ex.drain();

  EXPECT_EQ(peak.load(), 1);
}

TEST(SerialExecutorTest, ExceptionsPropagateAndDoNotKillWorker) {
  SerialExecutor ex;
  auto f1 = ex.submit([] {
    throw std::runtime_error("boom");
    return 0;
  });
  auto f2 = ex.submit([] { return 42; });

  EXPECT_THROW(f1.get(), std::runtime_error);
  EXPECT_EQ(f2.get(), 42);  // the worker survived the throw
}

TEST(SerialExecutorTest, DestructorRunsEverySubmittedTask) {
  std::atomic<int> ran{0};
  {
    auto ex = std::make_unique<SerialExecutor>();
    for (int i = 0; i < 100; ++i) ex->submit([&ran] { ran.fetch_add(1); });
    ex.reset();  // ~SerialExecutor() drains + joins
  }
  EXPECT_EQ(ran.load(), 100);
}

TEST(SerialExecutorTest, DrainWaitsForInflightTask) {
  SerialExecutor ex;
  std::atomic<int> done{0};
  auto f = ex.submit([&] {
    std::this_thread::sleep_for(50ms);
    done.fetch_add(1);
    return 42;
  });

  ex.drain();  // must not return until the in-flight task has completed
  EXPECT_EQ(done.load(), 1);
  EXPECT_EQ(f.get(), 42);
}

TEST(SerialExecutorTest, TaskCanSubmitMoreWork) {
  SerialExecutor ex;
  std::atomic<int> count{0};
  auto first = ex.submit([&] {
    ex.submit([&] {
      ex.submit([&] { count.fetch_add(1); });
      count.fetch_add(1);
    });
    count.fetch_add(1);
    return 1;
  });

  ex.drain();
  EXPECT_EQ(first.get(), 1);
  EXPECT_EQ(count.load(), 3);  // all three layers ran
}

TEST(SerialExecutorTest, ShutdownWaitsForQueuedWorkAndIsIdempotent) {
  SerialExecutor ex;
  std::atomic<int> completed{0};
  auto f = ex.submit([&] {
    std::this_thread::sleep_for(20ms);
    completed.fetch_add(1);
    return 7;
  });

  ex.shutdown();   // drains then joins
  EXPECT_EQ(f.get(), 7);
  EXPECT_EQ(completed.load(), 1);
  ex.shutdown();   // idempotent
}

TEST(SerialExecutorTest, SubmitAfterShutdownIsRejected) {
  SerialExecutor ex;
  ex.shutdown();

  // Contract: submit() after shutdown throws std::logic_error.
  // NOTE: with today's stub this passes spuriously, because the stub enqueue
  // throws regardless of shutdown state. Meaningful once the real queue exists.
  EXPECT_THROW(ex.submit([] {}), std::logic_error);
}

TEST(SerialExecutorTest, PendingReflectsQueuedWork) {
  SerialExecutor ex;
  ex.submit([&] { std::this_thread::sleep_for(20ms); });
  ex.submit([&] {});
  ex.submit([&] {});

  // At least the two not-yet-run tasks are pending; the in-flight one may or
  // may not have finished by the time we read.
  EXPECT_GE(ex.pending(), 2u);

  ex.drain();
  EXPECT_EQ(ex.pending(), 0u);
}