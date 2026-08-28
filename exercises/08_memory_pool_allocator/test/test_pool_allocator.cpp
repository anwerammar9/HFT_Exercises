#include "pool_allocator.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

// A type that counts its constructions/destructions so the tests can prove
// allocate() placement-news and deallocate() runs ~T.
struct Counting {
  static std::int64_t ctors;
  static std::int64_t dtors;

  std::uint64_t value = 0;
  explicit Counting(std::uint64_t v = 0) : value(v) { ++ctors; }
  ~Counting() { ++dtors; }
};
std::int64_t Counting::ctors = 0;
std::int64_t Counting::dtors = 0;

}  // namespace

// NOTE on the std-allocator wrapper (the plan marks it optional): NOT provided
// in this scaffold. `std::vector<T, PoolAllocator<T>>` needs `allocate(n)` to
// return n CONTIGUOUS slots, which fights the O(1) freelist contract. If you
// add one, do it as a separate adapter class and document which `n` it serves.

TEST(PoolAllocatorTest, AllocateUpToCapacityThenExhaust) {
  PoolAllocator<Counting> pool(8);
  std::vector<Counting*> slots;

  for (std::size_t i = 0; i < 8; ++i) {
    Counting* p = pool.allocate();
    ASSERT_NE(p, nullptr);
    p->value = i + 1;
    slots.push_back(p);
  }

  // Exhausted: the 9th allocation must fail (contract: nullptr, no throw).
  EXPECT_EQ(pool.allocate(), nullptr);

  // Every slot written above is still readable.
  for (std::size_t i = 0; i < slots.size(); ++i) EXPECT_EQ(slots[i]->value, i + 1);
}

TEST(PoolAllocatorTest, PointersAreAlignedForT) {
  // T with a 64-byte alignment: every handed-out slot must honour alignof(T).
  struct alignas(64) BigT {
    std::uint64_t pad = 0;
  };
  PoolAllocator<BigT> pool(16);
  std::vector<void*> slots;
  for (std::size_t i = 0; i < 16; ++i) {
    void* p = pool.allocate();
    ASSERT_NE(p, nullptr);
    slots.push_back(p);
  }
  for (void* p : slots) {
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 64u, 0u)
        << "slot must be 64-byte aligned for alignof(T) == 64";
  }
}

TEST(PoolAllocatorTest, DeallocatedSlotIsReused) {
  PoolAllocator<int> pool(4);
  int* a = pool.allocate();
  ASSERT_NE(a, nullptr);
  int* b = pool.allocate();
  ASSERT_NE(b, nullptr);
  int* c = pool.allocate();
  ASSERT_NE(c, nullptr);

  pool.deallocate(b);  // b returns to the freelist...

  int* d = pool.allocate();
  ASSERT_NE(d, nullptr);
  EXPECT_EQ(d, b);  // ...and the next alloc must hand out THAT very slot again.
}

TEST(PoolAllocatorTest, CtorDtorCountsMatchAllocDealloc) {
  Counting::ctors = 0;
  Counting::dtors = 0;

  {
    PoolAllocator<Counting> pool(16);
    PoolAllocator<Counting>::value_type* keep = nullptr;
    for (std::size_t i = 0; i < 12; ++i) {
      Counting* p = pool.allocate();
      ASSERT_NE(p, nullptr);
      if (i == 0) keep = p;  // one stays live when the pool is destroyed
    }
    EXPECT_EQ(Counting::ctors, 12);  // allocate() ran the constructor
    EXPECT_EQ(Counting::dtors, 0);

    if (keep != nullptr) pool.deallocate(keep);  // explicit destruction path
    EXPECT_EQ(Counting::dtors, 1);
  }
  // Destroying the pool must run ~T on the 11 survivors exactly once each.
  EXPECT_EQ(Counting::ctors, 12);
  EXPECT_EQ(Counting::dtors, 12);
}

TEST(PoolAllocatorTest, DeallocateNullptrIsNoOp) {
  PoolAllocator<int> pool(2);
  pool.deallocate(nullptr);  // must not crash / UB
  int* p = pool.allocate();
  EXPECT_NE(p, nullptr);
}

// STRETCH GOAL (the plan says the base version is single-threaded): the base
// PoolAllocator is intentionally NOT thread-safe, so this test ships DISABLED.
// Uncomment `DISABLED_` once allocate/deallocate are made thread-safe (internal
// spinlock, or a lock-free (Treiber-style) freelist) and document the choice.
TEST(PoolAllocatorTest, DISABLED_ConcurrentAllocDealloc) {
  // The 4 purses together hold exactly `capacity` live slots, so the pool
  // always has a free slot to serve the next rotation; this maximizes
  // freelist churn (the moment an ABA/races could bite) while never asking for
  // more than the arena can hold.
  constexpr int kWorkers = 4;
  constexpr int kPurseSize = 16;
  PoolAllocator<int> pool(kWorkers * kPurseSize);

  std::vector<std::vector<int*>> purses(kWorkers, std::vector<int*>(kPurseSize));

  std::vector<std::thread> workers;
  for (int r = 0; r < kWorkers; ++r) {
    workers.emplace_back([&, r] {
      for (int i = 0; i < kPurseSize; ++i) {
        purses[r][i] = pool.allocate();
        EXPECT_NE(purses[r][i], nullptr);
      }
      for (int i = 0; i < 5000; ++i) {
        const int slot = i % kPurseSize;
        int* held = purses[r][slot];
        if (held != nullptr) pool.deallocate(held);
        int* fresh = pool.allocate();
        EXPECT_NE(fresh, nullptr);
        purses[r][slot] = fresh;
      }
    });
  }
  for (auto& w : workers) w.join();
}