#include <gtest/gtest.h>

#include "arena_allocator.h"

#include <cstdint>
#include <vector>

namespace {

std::uintptr_t as_addr(const void* p) {
  return reinterpret_cast<std::uintptr_t>(p);
}

bool is_aligned(const void* p, std::size_t align) {
  return (as_addr(p) % align) == 0;
}

}  // namespace

TEST(ArenaAllocatorTest, BumpAllocationsNeverOverlap) {
  ArenaAllocator arena(256);
  std::vector<void*> ptrs;
  for (int i = 0; i < 8; ++i) {
    void* p = arena.allocate(16);
    ASSERT_NE(p, nullptr);
    ptrs.push_back(p);
  }
  for (std::size_t i = 0; i < ptrs.size(); ++i) {
    for (std::size_t j = i + 1; j < ptrs.size(); ++j) {
      EXPECT_NE(ptrs[i], ptrs[j]);
    }
  }
  // Bump discipline: strictly increasing addresses.
  for (std::size_t i = 1; i < ptrs.size(); ++i) {
    EXPECT_GT(ptrs[i], ptrs[i - 1]);
  }
}

TEST(ArenaAllocatorTest, RawAlignmentIsRespected) {
  ArenaAllocator arena(512);
  void* p = arena.allocate(17, 64);
  ASSERT_NE(p, nullptr);
  EXPECT_TRUE(is_aligned(p, 64));
}

TEST(ArenaAllocatorTest, AlignOfCustomTypesSurvivesAcrossAllocations) {
  ArenaAllocator arena(512);
  struct alignas(64) CacheLineThing {
    char bytes[64];
  };
  void* a = arena.allocate(sizeof(CacheLineThing), alignof(CacheLineThing));
  void* b = arena.allocate(sizeof(CacheLineThing), alignof(CacheLineThing));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_TRUE(is_aligned(a, 64));
  EXPECT_TRUE(is_aligned(b, 64));
  EXPECT_NE(a, b);
}

TEST(ArenaAllocatorTest, ChainsANewChunkWhenTight) {
  ArenaAllocator arena(64);  // tiny chunks force chaining
  void* a = arena.allocate(48);
  ASSERT_NE(a, nullptr);
  void* b = arena.allocate(48);
  ASSERT_NE(b, nullptr);
  EXPECT_NE(a, b);
  EXPECT_GT(arena.block_count(), 1u);
  EXPECT_GE(arena.capacity(), 128u);
  EXPECT_GE(arena.used_bytes(), 96u);
}

TEST(ArenaAllocatorTest, OversizeRequestGetsItsOwnChunk) {
  ArenaAllocator arena(64);
  void* p = arena.allocate(200, 8);
  ASSERT_NE(p, nullptr);
  EXPECT_TRUE(is_aligned(p, 8));
  EXPECT_GE(arena.used_bytes(), 200u);
}

TEST(ArenaAllocatorTest, ResetRewindsAndReusesTheSameAddresses) {
  ArenaAllocator arena(256);
  void* first = arena.allocate(32);
  ASSERT_NE(first, nullptr);
  ASSERT_GE(arena.used_bytes(), 32u);
  std::size_t cap = arena.capacity();

  arena.reset();
  EXPECT_EQ(arena.used_bytes(), 0u);
  EXPECT_EQ(arena.capacity(), cap);  // memory retained, not released

  void* again = arena.allocate(32);
  ASSERT_NE(again, nullptr);
  EXPECT_EQ(again, first);  // same address: no OS round-trip
}

TEST(ArenaAllocatorTest, ReleaseReturnsEverythingToZero) {
  ArenaAllocator arena(256);
  void* a = arena.allocate(16);
  ASSERT_NE(a, nullptr);
  ASSERT_GT(arena.used_bytes(), 0u);
  ASSERT_GT(arena.capacity(), 0u);

  arena.release();
  EXPECT_EQ(arena.used_bytes(), 0u);
  EXPECT_EQ(arena.capacity(), 0u);
  EXPECT_EQ(arena.block_count(), 0u);

  void* b = arena.allocate(16);  // still usable afterwards
  ASSERT_NE(b, nullptr);
  EXPECT_GT(arena.capacity(), 0u);
}

TEST(ArenaAllocatorTest, MarkAndRollbackFollowStackDiscipline) {
  ArenaAllocator arena(1024);
  void* a = arena.allocate(16);
  ASSERT_NE(a, nullptr);

  void* mark = arena.allocate_all();
  ASSERT_NE(mark, nullptr);
  std::size_t used_at_mark = arena.used_bytes();

  void* b = arena.allocate(16);
  ASSERT_NE(b, nullptr);
  EXPECT_GT(arena.used_bytes(), used_at_mark);

  arena.rollback_to(mark);
  EXPECT_EQ(arena.used_bytes(), used_at_mark);  // post-mark work undone
  void* c = arena.allocate(16);
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c, b);  // reuses exactly b's address
}

TEST(ArenaAllocatorTest, DeallocateIsANoop) {
  ArenaAllocator arena(256);
  void* a = arena.allocate(16);
  ASSERT_NE(a, nullptr);
  std::size_t before = arena.used_bytes();

  arena.deallocate(a);
  EXPECT_EQ(arena.used_bytes(), before);  // accounting untouched

  void* b = arena.allocate(16);
  ASSERT_NE(b, nullptr);
  EXPECT_NE(b, a);  // bump continues; the freed slot is NOT recycled
}

TEST(ArenaAllocatorTest, ZeroSizeIsStillAUniquePointer) {
  ArenaAllocator arena(256);
  void* a = arena.allocate(0);
  void* b = arena.allocate(0);
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_NE(a, b);
}

TEST(ArenaAllocatorTest, WholeLifetimeSurvivesRepeatedResetCycles) {
  ArenaAllocator arena(128);
  void* first = nullptr;
  for (int cycle = 0; cycle < 3; ++cycle) {
    for (int i = 0; i < 5; ++i) {
      void* p = arena.allocate(8);
      ASSERT_NE(p, nullptr);
      if (cycle == 0 && i == 0) first = p;
    }
    arena.reset();
  }
  ASSERT_NE(first, nullptr);
  void* again = arena.allocate(8);
  EXPECT_EQ(again, first);  // every cycle reuses the same low address
}