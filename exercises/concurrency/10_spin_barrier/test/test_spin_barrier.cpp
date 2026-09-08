#include "spin_barrier.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// NOTE: a no-op stub spin barrier lets threads through the moment they call
// wait(), which is what makes WaitBlocksUntilAllNThreadsArrive and the
// generation check below RED against the stub. Every test terminates promptly
// even against the stubs (nothing here waits on a future).

TEST(SpinBarrierTest, ZeroSizeIsRejected) {
  EXPECT_THROW(SpinBarrier{0}, std::invalid_argument);
}

TEST(SpinBarrierTest, SingleThreadPassesThrough) {
  SpinBarrier barrier(1);
  barrier.wait();  // the only thread IS the last arriver -> returns
  EXPECT_EQ(barrier.generation(), 1u);
}

TEST(SpinBarrierTest, WaitBlocksUntilAllNThreadsArrive) {
  // Discriminating test: two threads reaching the barrier must NOT pass until
  // the third arrives. A no-op stub lets them through immediately -> red;
  // the real barrier holds them -> the checks below only pass for real.
  constexpr int kThreads = 3;
  SpinBarrier barrier(kThreads);
  std::atomic<int> arrived{0};
  std::atomic<int> passed{0};

  std::vector<std::thread> workers;
  for (int i = 0; i < kThreads - 1; ++i) {
    workers.emplace_back([&] {
      arrived.fetch_add(1);
      barrier.wait();
      passed.fetch_add(1);
    });
  }

  // Let the first two threads reach the barrier; they must stay blocked.
  std::this_thread::sleep_for(50ms);
  EXPECT_EQ(arrived.load(), kThreads - 1);  // sanity: they did get there
  EXPECT_EQ(passed.load(), 0);              // ...but were NOT released

  barrier.wait();  // the final thread arrives -> everyone proceeds
  for (auto& w : workers) w.join();

  EXPECT_EQ(passed.load(), kThreads - 1);
}

TEST(SpinBarrierTest, AllThreadsSeeFullArrivalAfterRelease) {
  // Everything a thread did BEFORE wait() must be visible to every thread
  // AFTER wait() on the same round. Given N threads each doing fetch_add on a
  // shared counter before waiting, any thread that passed the barrier must
  // observe a value >= N.
  constexpr int kThreads = 6;
  SpinBarrier barrier(kThreads);
  std::atomic<int> next_ticket{0};
  struct Slot {
    int before = 0;
    int after = 0;
  };
  std::vector<Slot> slots(kThreads);

  std::vector<std::thread> workers;
  for (int i = 0; i < kThreads; ++i) {
    workers.emplace_back([&, i] {
      slots[i].before = next_ticket.fetch_add(1);
      barrier.wait();
      slots[i].after = next_ticket.load();
    });
  }
  for (auto& w : workers) w.join();

  for (int i = 0; i < kThreads; ++i) {
    EXPECT_GE(slots[i].after, kThreads)
        << "thread " << i << " passed before all arrivals were visible";
  }
}

TEST(SpinBarrierTest, ReusableAcrossMultipleRounds) {
  constexpr int kThreads = 3;
  constexpr int kRounds = 5;
  SpinBarrier barrier(kThreads);
  // Count arrivals before each wait() so the main thread can observe them.
  std::vector<std::atomic<int>> counts(kThreads - 1);

  std::vector<std::thread> workers;
  for (int i = 0; i < kThreads - 1; ++i) {
    workers.emplace_back([&, i] {
      for (int r = 0; r < kRounds; ++r) {
        ++counts[i];
        barrier.wait();
      }
    });
  }

  // Main thread participates as one of the arrivals, so it can observe the
  // workers only after the round has completed.
  for (int r = 0; r < kRounds; ++r) {
    barrier.wait();
    for (int i = 0; i < kThreads - 1; ++i) {
      // Each worker's round-r increment happened-before the round-r release,
      // so every count must be at least r + 1 here (monotonic, relaxed OK).
      EXPECT_GE(counts[i].load(std::memory_order_relaxed), r + 1);
    }
  }
  for (auto& w : workers) w.join();

  EXPECT_EQ(barrier.generation(), static_cast<std::size_t>(kRounds));
}