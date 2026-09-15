#include "arena_gc.h"

#include <gtest/gtest.h>

#include <string>

namespace {

struct Link {
  ArenaPtr<Link> next;
  int value = 0;
  explicit Link(int v = 0) : value(v) {}
  void trace(ArenaGcHeap::Tracer& tr) const { tr.visit(next.slot()); }
};

struct Leaf {
  int value = 0;
  explicit Leaf(int v = 0) : value(v) {}
  // No trace(): a leaf payload.
};

}  // namespace

TEST(ArenaGcTest, StartsEmpty) {
  ArenaGcHeap heap(16);
  EXPECT_EQ(heap.live(), 0u);
  EXPECT_EQ(heap.capacity(), 16u);
  EXPECT_EQ(heap.roots(), 0u);
  EXPECT_EQ(heap.collect(), 0u);
}

TEST(ArenaGcTest, RootedObjectSurvivesCollect) {
  ArenaGcHeap heap(16);
  auto h = heap.allocate<Leaf>(7);
  ASSERT_TRUE(h) << "allocation must succeed";
  auto r = heap.root(h);
  EXPECT_EQ(heap.roots(), 1u);
  EXPECT_EQ(heap.collect(), 0u) << "rooted object is not reclaimed";
  EXPECT_TRUE(heap.is_live(h));
  EXPECT_EQ(heap.live(), 1u);
  EXPECT_EQ(h->value, 7);
}

TEST(ArenaGcTest, UnrootedTailIsReclaimed) {
  ArenaGcHeap heap(16);
  auto a = heap.allocate<Leaf>(1);
  auto b = heap.allocate<Leaf>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  auto ra = heap.root(a);  // only a is rooted; b is dead tail
  EXPECT_EQ(heap.collect(), 1u) << "dead tail pops";
  EXPECT_TRUE(heap.is_live(a));
  EXPECT_FALSE(heap.is_live(b));
  EXPECT_EQ(heap.live(), 1u);
}

TEST(ArenaGcTest, InteriorGarbageStaysPinned) {
  ArenaGcHeap heap(16);
  auto a = heap.allocate<Leaf>(1);  // interior, will die...
  auto b = heap.allocate<Leaf>(2);  // ...but the tail stays live
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  auto rb = heap.root(b);  // root only the tail
  EXPECT_EQ(heap.collect(), 0u) << "tail is live: nothing pops";
  EXPECT_TRUE(heap.is_live(a)) << "interior garbage pinned until tail drains";
  EXPECT_EQ(heap.live(), 2u);
}

TEST(ArenaGcTest, ReachableChainSurvives) {
  ArenaGcHeap heap(16);
  auto n1 = heap.allocate<Link>(1);
  auto n2 = heap.allocate<Link>(2);
  auto n3 = heap.allocate<Link>(3);
  ASSERT_TRUE(n1);
  ASSERT_TRUE(n2);
  ASSERT_TRUE(n3);
  n1->next = n2;
  n2->next = n3;
  auto r = heap.root(n1);  // whole chain reachable from one root
  EXPECT_EQ(heap.collect(), 0u);
  EXPECT_TRUE(heap.is_live(n1));
  EXPECT_TRUE(heap.is_live(n2));
  EXPECT_TRUE(heap.is_live(n3));
  EXPECT_EQ(heap.live(), 3u);
}

TEST(ArenaGcTest, RootReleaseDrainsOnNextCollect) {
  ArenaGcHeap heap(16);
  auto a = heap.allocate<Leaf>(1);
  auto b = heap.allocate<Leaf>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  {
    auto ra = heap.root(a);
    auto rb = heap.root(b);
    EXPECT_EQ(heap.collect(), 0u);
  }  // roots drop here
  EXPECT_EQ(heap.roots(), 0u);
  EXPECT_EQ(heap.collect(), 2u) << "unrooted tail drains completely";
  EXPECT_EQ(heap.live(), 0u);
}

TEST(ArenaGcTest, ExhaustedArenaReturnsNull) {
  ArenaGcHeap heap(2);
  auto a = heap.allocate<Leaf>(1);
  auto b = heap.allocate<Leaf>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  auto c = heap.allocate<Leaf>(3);
  EXPECT_FALSE(c) << "bump limit enforced: null handle when full";
  EXPECT_EQ(heap.live(), 2u);
}

TEST(ArenaGcTest, ResetBulkReclaimsAndReuses) {
  ArenaGcHeap heap(4);
  auto a = heap.allocate<Leaf>(1);
  ASSERT_TRUE(a);
  auto r = heap.root(a);
  heap.reset();
  EXPECT_EQ(heap.live(), 0u);
  EXPECT_EQ(heap.roots(), 0u);
  EXPECT_EQ(heap.capacity(), 4u) << "capacity retained, memory reused";
  auto b = heap.allocate<Leaf>(9);
  ASSERT_TRUE(b);
  EXPECT_EQ(b->value, 9);
  EXPECT_EQ(heap.live(), 1u);
}

TEST(ArenaGcTest, CollectIsRepeatableAndExact) {
  ArenaGcHeap heap(16);
  for (int i = 0; i < 5; ++i) {
    auto h = heap.allocate<Leaf>(i);
    ASSERT_TRUE(h);
  }
  EXPECT_EQ(heap.live(), 5u);
  EXPECT_EQ(heap.collect(), 5u) << "nothing rooted: whole tail pops";
  EXPECT_EQ(heap.live(), 0u);
  EXPECT_EQ(heap.collect(), 0u) << "second collect is a no-op";
}
