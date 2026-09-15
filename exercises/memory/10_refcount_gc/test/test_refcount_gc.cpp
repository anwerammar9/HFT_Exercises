#include "refcount_gc.h"

#include <gtest/gtest.h>

#include <string>

namespace {

struct Node {
  static int alive;
  RcPtr<Node> next;
  int value = 0;
  explicit Node(int v = 0) : value(v) { ++alive; }
  ~Node() { --alive; }
  void trace(RcTracer& tr) const { tr.visit(next.node()); }
};
int Node::alive = 0;

struct Leaf {
  static int alive;
  int value = 0;
  explicit Leaf(int v = 0) : value(v) { ++alive; }
  ~Leaf() { --alive; }
};
int Leaf::alive = 0;

void reset_counters() {
  Node::alive = 0;
  Leaf::alive = 0;
}

}  // namespace

TEST(RefcountGcTest, StartsEmpty) {
  RcHeap heap;
  EXPECT_EQ(heap.live(), 0u);
  EXPECT_EQ(heap.roots(), 0u);
  EXPECT_EQ(heap.collect_cycles(), 0u);
}

TEST(RefcountGcTest, MakeOwnsWithCountOne) {
  reset_counters();
  RcHeap heap;
  {
    auto p = heap.make<Leaf>(7);
    ASSERT_TRUE(p);
    EXPECT_EQ(p.use_count(), 1u);
    EXPECT_EQ(p->value, 7);
    EXPECT_EQ(Leaf::alive, 1);
    EXPECT_EQ(heap.live(), 1u);
  }
  EXPECT_EQ(Leaf::alive, 0) << "last handle destroys immediately, no collect";
  EXPECT_EQ(heap.live(), 0u);
}

TEST(RefcountGcTest, CopySharesAndDropReleases) {
  reset_counters();
  RcHeap heap;
  auto a = heap.make<Leaf>(1);
  ASSERT_TRUE(a);
  {
    RcPtr<Leaf> b = a;
    EXPECT_EQ(a.use_count(), 2u);
    EXPECT_EQ(b.use_count(), 2u);
    EXPECT_EQ(Leaf::alive, 1);
  }
  EXPECT_EQ(a.use_count(), 1u);
  EXPECT_EQ(Leaf::alive, 1);
}

TEST(RefcountGcTest, MoveTransfersWithoutBump) {
  RcHeap heap;
  auto a = heap.make<Leaf>(3);
  ASSERT_TRUE(a);
  Leaf* raw = a.get();
  RcPtr<Leaf> b(std::move(a));
  EXPECT_FALSE(a);
  EXPECT_EQ(a.use_count(), 0u);
  ASSERT_TRUE(b);
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(b.use_count(), 1u);
}

TEST(RefcountGcTest, AcyclicChainDiesImmediately) {
  reset_counters();
  RcHeap heap;
  {
    auto n1 = heap.make<Node>(1);
    auto n2 = heap.make<Node>(2);
    ASSERT_TRUE(n1);
    ASSERT_TRUE(n2);
    n1->next = n2;  // chain: n1 -> n2
    EXPECT_EQ(Node::alive, 2);
  }  // handles die: cascade destroys both WITHOUT any collection
  EXPECT_EQ(Node::alive, 0);
  EXPECT_EQ(heap.live(), 0u);
  EXPECT_EQ(heap.collect_cycles(), 0u) << "nothing left for the collector";
}

TEST(RefcountGcTest, CycleLingersUntilCollected) {
  reset_counters();
  RcHeap heap;
  {
    auto a = heap.make<Node>(1);
    auto b = heap.make<Node>(2);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    a->next = b;
    b->next = a;  // cycle: counts never reach zero on their own
  }  // handles drop, but the island survives...
  EXPECT_EQ(Node::alive, 2) << "refcounting alone cannot free cycles";
  EXPECT_EQ(heap.live(), 2u);
  EXPECT_EQ(heap.collect_cycles(), 2u) << "the cycle collector frees the island";
  EXPECT_EQ(Node::alive, 0);
  EXPECT_EQ(heap.live(), 0u);
}

TEST(RefcountGcTest, RootedCycleSurvivesCollect) {
  reset_counters();
  RcHeap heap;
  auto a = heap.make<Node>(1);
  auto b = heap.make<Node>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  a->next = b;
  b->next = a;
  auto r = heap.root(a);
  EXPECT_EQ(heap.collect_cycles(), 0u);
  EXPECT_EQ(Node::alive, 2);
  auto up = r.lock();
  ASSERT_TRUE(up);
  EXPECT_EQ(up->value, 1);
}

TEST(RefcountGcTest, ExternalHandleKeepsCycleAlive) {
  reset_counters();
  RcHeap heap;
  auto a = heap.make<Node>(1);
  auto b = heap.make<Node>(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  a->next = b;
  b->next = a;
  b.reset();  // drop one handle; the cycle hangs off 'a' only
  EXPECT_EQ(heap.collect_cycles(), 0u) << "externally held cycle survives";
  EXPECT_EQ(Node::alive, 2);
  a.reset();
  EXPECT_EQ(Node::alive, 2) << "still cyclic garbage after the last drop";
  EXPECT_EQ(heap.collect_cycles(), 2u);
  EXPECT_EQ(Node::alive, 0);
}

TEST(RefcountGcTest, ResetReleasesImmediately) {
  reset_counters();
  RcHeap heap;
  auto p = heap.make<Leaf>(4);
  ASSERT_TRUE(p);
  auto q = p;
  p.reset();
  EXPECT_EQ(Leaf::alive, 1);
  EXPECT_EQ(q.use_count(), 1u);
  q.reset();
  EXPECT_FALSE(q);
  EXPECT_EQ(Leaf::alive, 0);
}

TEST(RefcountGcTest, RootLockUpgrades) {
  RcHeap heap;
  auto p = heap.make<Leaf>(8);
  ASSERT_TRUE(p);
  auto r = heap.root(p);
  auto up = r.lock();
  ASSERT_TRUE(up);
  EXPECT_EQ(up.get(), p.get());
  EXPECT_EQ(p.use_count(), 2u);
}
