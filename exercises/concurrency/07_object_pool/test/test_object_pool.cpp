#include "object_pool.h"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

namespace {

int constexpr kMaxAttempts = 2000;

TEST(ObjectPoolTest, CapacityGetter) {
  ObjectPool<int> pool(7);
  EXPECT_EQ(pool.capacity(), 7u);
}

TEST(ObjectPoolTest, AcquireReturnsObjectsUpToCapacity) {
  ObjectPool<int> pool(4);
  std::vector<ObjectPool<int>::Handle> handles;

  for (int i = 0; i < 4; ++i) {
    auto h = pool.acquire();
    EXPECT_TRUE(static_cast<bool>(h));  // stub: empty -> red
    handles.push_back(std::move(h));
  }

  std::unordered_set<int*> addrs;
  for (auto& h : handles) addrs.insert(h.get());
  EXPECT_EQ(addrs.size(), 4u);  // all distinct slots

  auto fifth = pool.acquire();
  EXPECT_FALSE(static_cast<bool>(fifth));  // exhausted
}

TEST(ObjectPoolTest, ReleasedSlotIsReused) {
  ObjectPool<int> pool(2);
  auto a = pool.acquire();
  auto b = pool.acquire();
  ASSERT_TRUE(static_cast<bool>(a));
  ASSERT_TRUE(static_cast<bool>(b));

  int* first_addr = a.get();
  a.reset();  // return a to the pool

  auto c = pool.acquire();
  EXPECT_TRUE(static_cast<bool>(c));
  EXPECT_EQ(c.get(), first_addr);  // SAME slot recycled
}

TEST(ObjectPoolTest, RaiiHandleRecyclesOnScopeExit) {
  ObjectPool<int> pool(1);
  int* addr = nullptr;
  {
    auto h = pool.acquire();
    ASSERT_TRUE(static_cast<bool>(h));
    addr = h.get();
    *h = 42;
  }  // handle destroyed -> object back in the pool

  auto h2 = pool.acquire();
  EXPECT_TRUE(static_cast<bool>(h2));
  EXPECT_EQ(h2.get(), addr);
}

TEST(ObjectPoolTest, AcquireFreshensObject) {
  ObjectPool<int> pool(1);
  {
    auto h = pool.acquire();
    ASSERT_TRUE(static_cast<bool>(h));
    *h = 12345;
  }
  {
    auto h = pool.acquire();
    ASSERT_TRUE(static_cast<bool>(h));
    EXPECT_EQ(*h, 0);  // re-acquired slot is default-constructed
  }
}

TEST(ObjectPoolTest, MoveSemanticsKeepSingleRecycle) {
  ObjectPool<int> pool(1);

  auto h = pool.acquire();
  ASSERT_TRUE(static_cast<bool>(h));
  int* addr = h.get();

  ObjectPool<int>::Handle m = std::move(h);
  EXPECT_FALSE(static_cast<bool>(h));  // source is empty after the move
  EXPECT_EQ(m.get(), addr);

  ObjectPool<int>::Handle m2;
  m2 = std::move(m);
  EXPECT_FALSE(static_cast<bool>(m));
  EXPECT_EQ(m2.get(), addr);

  m2.reset();  // exactly ONE recycle happens
  auto h2 = pool.acquire();
  EXPECT_TRUE(static_cast<bool>(h2));
  EXPECT_EQ(h2.get(), addr);
}

TEST(ObjectPoolTest, HandleReleaseDetachesFromPool) {
  ObjectPool<int> pool(1);
  auto h = pool.acquire();
  ASSERT_TRUE(static_cast<bool>(h));

  int* leaked = h.release();   // caller keeps it; pool never gets it back
  EXPECT_EQ(h.get(), nullptr);

  auto h2 = pool.acquire();
  EXPECT_FALSE(static_cast<bool>(h2));  // the only slot is gone forever
  (void)leaked;
}

TEST(ObjectPoolTest, ZeroCapacityPoolNeverAcquires) {
  ObjectPool<int> pool(0);
  auto h = pool.acquire();
  EXPECT_FALSE(static_cast<bool>(h));
}

namespace {

struct Tracked {
  static inline std::atomic<int> ctor_calls{0};
  static inline std::atomic<int> dtor_calls{0};

  Tracked() { ctor_calls.fetch_add(1, std::memory_order_relaxed); }
  ~Tracked() { dtor_calls.fetch_add(1, std::memory_order_relaxed); }
  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;
};

}  // namespace

TEST(ObjectPoolTest, ConstructDestructExactlyBalanced) {
  Tracked::ctor_calls = 0;
  Tracked::dtor_calls = 0;

  constexpr int kCycles = 100;
  {
    ObjectPool<Tracked> pool(2);
    for (int i = 0; i < kCycles; ++i) {
      auto h = pool.acquire();
      ASSERT_TRUE(static_cast<bool>(h));
    }  // each cycle: one construct_at on acquire, one destroy_at on release
  }

  EXPECT_EQ(Tracked::ctor_calls.load(), kCycles);  // stub: 0 -> red
  EXPECT_EQ(Tracked::dtor_calls.load(), kCycles);
}

TEST(ObjectPoolTest, ConcurrentExclusiveAcquireStress) {
  constexpr std::size_t kPool = 8;
  constexpr int kThreads = 8;
  constexpr int kCycles = 100;

  ObjectPool<int> pool(kPool);
  std::atomic<int> successes{0};
  std::atomic<int> in_use{0};
  std::atomic<int> peak_in_use{0};

  std::mutex mu;
  std::unordered_set<void*> live_addrs;

  auto worker = [&]() {
    for (int i = 0; i < kCycles; ++i) {
      ObjectPool<int>::Handle h;
      for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        h = pool.acquire();
        if (h) break;
        if ((attempt & 1023) == 1023) std::this_thread::yield();
      }
      if (!h) continue;  // counted below via `successes` (stub: always gives up)

      EXPECT_EQ(*h, 0);  // slot fresh even after heavy reuse

      {
        std::lock_guard<std::mutex> g(mu);
        EXPECT_TRUE(live_addrs.insert(h.get()).second);  // never two holders
      }

      int cur = in_use.fetch_add(1) + 1;
      int peak = peak_in_use.load(std::memory_order_relaxed);
      while (peak < cur && !peak_in_use.compare_exchange_weak(peak, cur,
                                                              std::memory_order_relaxed)) {
      }
      EXPECT_LE(cur, static_cast<int>(kPool));  // never exceed capacity

      *h = i;  // simulated use

      in_use.fetch_sub(1);
      {
        std::lock_guard<std::mutex> g(mu);
        live_addrs.erase(h.get());
      }
      successes.fetch_add(1, std::memory_order_relaxed);
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) threads.emplace_back(worker);
  for (auto& t : threads) t.join();

  EXPECT_GT(successes.load(), 0);                       // stub: 0 -> red
  EXPECT_EQ(successes.load(), kThreads * kCycles);      // every attempt got one
  EXPECT_LE(peak_in_use.load(), static_cast<int>(kPool));
}

}  // namespace