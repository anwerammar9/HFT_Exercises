#include "rw_spinlock.h"
#include "seqlock.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

// A value type the writer keeps internally consistent: a + b == sum.
struct Snap {
  std::uint64_t a = 0;
  std::uint64_t b = 0;
  std::uint64_t sum = 0;

  bool is_state_a() const { return a == 1 && b == 2 && sum == 3; }
  bool is_state_b() const { return a == 2 && b == 4 && sum == 6; }
  bool is_initial() const { return a == 0 && b == 0 && sum == 0; }

  // A reader must only ever observe a fully-written snapshot: either the
  // pre-write default, or one of the two states with its invariant intact.
  bool consistent() const {
    return is_initial() || ((a + b == sum) && (is_state_a() || is_state_b()));
  }
};

constexpr Snap kStateA{1, 2, 3};
constexpr Snap kStateB{2, 4, 6};

}  // namespace

TEST(SeqlockTest, SingleWriterSingleReaderNeverTorn) {
  Seqlock<Snap> sl;

  std::atomic<bool> go{false};
  std::atomic<std::uint64_t> torn{0};
  std::atomic<std::uint64_t> real_states{0};

  std::thread writer([&] {
    sl.write(kStateA);                      // publish once before readers start
    go.store(true, std::memory_order_release);
    for (std::uint64_t i = 0; i < 200'000; ++i) {
      sl.write(i % 2 == 0 ? kStateA : kStateB);
    }
  });

  std::thread reader([&] {
    while (!go.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    for (std::uint64_t i = 0; i < 500'000; ++i) {
      const Snap s = sl.read();
      if (!s.consistent()) {
        torn.fetch_add(1);
        return;  // first torn read is enough evidence
      }
      if (s.is_state_a() || s.is_state_b()) {
        real_states.fetch_add(1);
      }
    }
  });

  writer.join();
  reader.join();

  EXPECT_EQ(torn.load(), 0u);
  EXPECT_GT(real_states.load(), 0u);  // stub returns only the default => red
}

TEST(SeqlockTest, SingleWriterMultipleReadersStress) {
  Seqlock<Snap> sl;

  constexpr int kReaders = 4;
  constexpr std::uint64_t kReadsPerReader = 250'000;  // 1M reads total
  std::atomic<std::uint64_t> torn{0};
  std::atomic<std::uint64_t> real_states{0};

  std::thread writer([&] {
    for (std::uint64_t i = 1; i <= 600'000; ++i) {
      // Alternate in runs to stress the copy window rather than flip each write.
      sl.write((i / 64) % 2 == 0 ? kStateA : kStateB);
    }
  });

  std::vector<std::thread> readers;
  readers.reserve(kReaders);
  for (int r = 0; r < kReaders; ++r) {
    readers.emplace_back([&] {
      for (std::uint64_t i = 0; i < kReadsPerReader; ++i) {
        const Snap s = sl.read();
        if (!s.consistent()) {
          torn.fetch_add(1);
          return;
        }
        if (s.is_state_a() || s.is_state_b()) {
          real_states.fetch_add(1);
        }
      }
    });
  }

  for (auto& rd : readers) rd.join();
  writer.join();

  EXPECT_EQ(torn.load(), 0u);
  EXPECT_GT(real_states.load(), 0u);  // stub returns only the default => red
}

TEST(RWSpinLockTest, MultipleConcurrentReadersAllowed) {
  RWSpinLock lk;

  constexpr int kReaders = 4;
  std::atomic<int> active{0};
  std::atomic<int> peak{0};
  std::atomic<bool> begin{false};

  std::vector<std::thread> readers;
  readers.reserve(kReaders);
  for (int r = 0; r < kReaders; ++r) {
    readers.emplace_back([&] {
      while (!begin.load()) std::this_thread::yield();
      for (int i = 0; i < 20'000; ++i) {
        lk.lock_read();
        const int cur = active.fetch_add(1) + 1;
        for (int p = peak.load(); cur > p && !peak.compare_exchange_weak(p, cur);) {}
        active.fetch_sub(1);
        lk.unlock_read();
      }
    });
  }

  begin.store(true);
  for (auto& rd : readers) rd.join();

  EXPECT_GT(peak.load(), 1);  // need at least two readers inside simultaneously
}

TEST(RWSpinLockTest, WriterExclusiveNoReaderWriterOverlap) {
  // The writer publishes {gen, payload=gen*2} under the write lock and widens
  // the window between the two writes on purpose: a reader under a correct read
  // lock can NEVER observe the intermediate (gen, stale payload) torn state.
  // With the no-op stub both lock paths are no-ops, so the torn state gets
  // observed and the test fails red.
  RWSpinLock lk;

  struct Data {
    std::atomic<std::uint64_t> gen{0};
    std::atomic<std::uint64_t> payload{0};
  };
  Data data;

  std::atomic<bool> begin{false};
  std::atomic<std::uint64_t> torn{0};

  std::thread writer([&] {
    while (!begin.load()) std::this_thread::yield();
    for (std::uint64_t g = 1; g <= 100'000; ++g) {
      lk.lock_write();
      data.gen.store(g, std::memory_order_relaxed);
      std::this_thread::yield();  // widen the exposed window
      data.payload.store(g * 2, std::memory_order_relaxed);
      lk.unlock_write();
    }
  });

  std::vector<std::thread> readers;
  constexpr int kReaders = 4;
  readers.reserve(kReaders);
  for (int r = 0; r < kReaders; ++r) {
    readers.emplace_back([&] {
      while (!begin.load()) std::this_thread::yield();
      for (std::uint64_t i = 0; i < 300'000; ++i) {
        lk.lock_read();
        // payload read before gen: a (new gen, old payload) sample is a torn
        // observation. Atomic fields keep TSan quiet while still detecting
        // cross-field tearing, which is exactly the seqlock guarantee.
        const std::uint64_t p =
            data.payload.load(std::memory_order_seq_cst);
        const std::uint64_t g = data.gen.load(std::memory_order_seq_cst);
        const bool ok = p == g * 2;
        if (!ok) torn.fetch_add(1);
        lk.unlock_read();
        if (torn.load() > 0) return;
      }
    });
  }

  begin.store(true);
  for (auto& rd : readers) rd.join();
  writer.join();

  EXPECT_EQ(torn.load(), 0u);
}