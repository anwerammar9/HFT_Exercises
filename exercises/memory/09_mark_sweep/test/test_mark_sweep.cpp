#include "mark_sweep.h"

#include <gtest/gtest.h>

#include <string>

namespace {

struct Link {
  GcPtr<Link> next;
  int value = 0;
  explicit Link(int v = 0) : value(v) {}
  void trace(GcTracer& tr) const { tr.visit(next.slot_ptr()); }
};

struct Leaf {
  int value = 0;
  explicit Leaf(int v = 0) : value(v) {}
};

}  // namespace

TEST(MarkSweepTest, StartsEmpty) {
  GcHeap heap;
  EXPECT_EQ(heap.live(), 0u);
  EXPECT_EQ(heap.roots(), 0u);
  EXPECT_EQ(heap.collect(), 0u);
}

TEST(MarkSweepTest, RootedObjectSurvives) {
  GcHeap heap;
  auto h = heap.allocate<Leaf>(7);
  ASSERT_TRUE(h) << "allocation must succeed";
  auto r = heap.root(h);
  EXPECT_EQ(heap.collect(), 0u);
  EXPECT_TRUE(heap.is_live(h));
  EXPECT_EQ(heap.live(), 1u);
  EXPECT_EQ(h->value, 7);
}

TEST(MarkSweepTest, UnrootedObjectsSweptAnywhere) {
  GcHeap heap;
  auto a = heap.allocate<Leaf>(1);
  auto keep = heap.allocate<Leaf>(2);
  auto b = heap.allocate<Leaf>(3);
  ASSERT_TRUE(a);
  ASSERT_TRUE(keep);
  ASSERT_TRUE(b);
  auto r = heap.root(keep);  // garbage on BOTH sides of the survivor
  EXPECT_EQ(heap.collect(), 2u) << "sweep reclaims anywhere, not just the tail";
  EXPECT_FALSE(heap.is_live(a));
  EXPECT_TRUE(heap.is_live(keep));
  EXPECT_FALSE(heap.is_live(b));
  EXPECT_EQ(heap.live(), 1u);
}

TEST(MarkSweepTest, ReachableChainSurvives) {
  GcHeap heap;
  auto n1 = heap.allocate<Link>(1);
  auto n2 = heap.allocate<Link>(2);
  auto n3 = heap.allocate<Link>(3);
  ASSERT_TRUE(n1);
  ASSERT_TRUE(n2);
  ASSERT_TRUE(n3);
  n1->next = n2;
  n2->next = n3;
  auto r = heap.root(n1);
  EXPECT_EQ(heap.collect(), 0u);
  EXPECT_TRUE(heap.is_live(n3)) << "transitively reachable survives";
  EXPECT_EQ(heap.live(), 3u);
}

TEST(MarkSweepTest, UnreachableCycleIsReclaimed) {
  GcHeap heap;
  auto a = heap.allocate<Link>(1);
  auto b = heap.allocate<Link>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  a->next = b;
  b->next = a;  // cycle, no root: refcounting could never free this
  EXPECT_EQ(heap.collect(), 2u) << "cycles die with everything else";
  EXPECT_FALSE(heap.is_live(a));
  EXPECT_FALSE(heap.is_live(b));
  EXPECT_EQ(heap.live(), 0u);
}

TEST(MarkSweepTest, RootedCycleSurvives) {
  GcHeap heap;
  auto a = heap.allocate<Link>(1);
  auto b = heap.allocate<Link>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  a->next = b;
  b->next = a;
  auto r = heap.root(a);  // rooted cycle stays
  EXPECT_EQ(heap.collect(), 0u);
  EXPECT_TRUE(heap.is_live(a));
  EXPECT_TRUE(heap.is_live(b));
}

TEST(MarkSweepTest, RootReleaseFreesOnNextCollect) {
  GcHeap heap;
  auto a = heap.allocate<Leaf>(1);
  ASSERT_TRUE(a);
  {
    auto r = heap.root(a);
    EXPECT_EQ(heap.collect(), 0u);
  }
  EXPECT_EQ(heap.roots(), 0u);
  EXPECT_EQ(heap.collect(), 1u);
  EXPECT_EQ(heap.live(), 0u);
}

TEST(MarkSweepTest, SharedChildSurvivesOneRoot) {
  GcHeap heap;
  auto top = heap.allocate<Link>(0);
  auto shared = heap.allocate<Link>(9);
  ASSERT_TRUE(top);
  ASSERT_TRUE(shared);
  top->next = shared;  // reachable via edge AND via its own root
  {
    auto r1 = heap.root(top);
    auto r2 = heap.root(shared);
    EXPECT_EQ(heap.roots(), 2u);
    EXPECT_EQ(heap.collect(), 0u);
    EXPECT_EQ(heap.live(), 2u);
  }  // both roots drop
  EXPECT_EQ(heap.collect(), 2u);
  EXPECT_FALSE(heap.is_live(shared));
  EXPECT_EQ(heap.live(), 0u);
}
