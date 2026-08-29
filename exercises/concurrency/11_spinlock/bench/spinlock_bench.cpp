// Optional benchmark: contends N threads over a shared SpinLock on a counter.
// Run with:
//   ./build/.../spinlock_bench --benchmark_min_time=2s
//
// Measures the live implementation in src/spinlock.cpp (TAS with backoff). If
// you later re-stub the lock, it runs effectively uncontended; the naive-TAS /
// TTAS reference in the note above is why it lives in a bench, not the
// exercise. It is not part of the graded suite.

#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>
#include <vector>

#include "spinlock.h"

namespace {

void contended_increment(benchmark::State& state, SpinLock* lk, int threads) {
  std::int64_t counter = 0;
  std::atomic<bool> go{false};
  while (state.KeepRunning()) {
    go.store(false);
    counter = 0;
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t) {
      ts.emplace_back([&] {
        while (!go.load(std::memory_order_relaxed)) std::this_thread::yield();
        for (int i = 0; i < 100; ++i) {
          std::lock_guard<SpinLock> guard(*lk);
          (void)i;  // keep some work inside the critical section
          ++counter;
        }
      });
    }
    go.store(true, std::memory_order_release);
    for (auto& t : ts) t.join();
  }
}

}  // namespace

static void BM_SpinLock_1Thread(benchmark::State& state) {
  SpinLock lk;
  contended_increment(state, &lk, 1);
}
BENCHMARK(BM_SpinLock_1Thread);

static void BM_SpinLock_2Threads(benchmark::State& state) {
  SpinLock lk;
  contended_increment(state, &lk, 2);
}
BENCHMARK(BM_SpinLock_2Threads);

static void BM_SpinLock_4Threads(benchmark::State& state) {
  SpinLock lk;
  contended_increment(state, &lk, 4);
}
BENCHMARK(BM_SpinLock_4Threads);

static void BM_SpinLock_8Threads(benchmark::State& state) {
  SpinLock lk;
  contended_increment(state, &lk, 8);
}
BENCHMARK(BM_SpinLock_8Threads);

BENCHMARK_MAIN();