#include "shared_ptr.h"

#include <gtest/gtest.h>

#include <thread>
#include <utility>
#include <vector>

namespace {

struct Tracked {
  static int alive;
  static int dtors;
  int value = 0;
  explicit Tracked(int v = 0) : value(v) { ++alive; }
  ~Tracked() {
    --alive;
    ++dtors;
  }
};
int Tracked::alive = 0;
int Tracked::dtors = 0;

struct CountingDeleter {
  static int calls;
  void operator()(int* p) const {
    ++calls;
    delete p;
  }
};
int CountingDeleter::calls = 0;

struct Base {
  virtual ~Base() = default;
  virtual int id() const { return 1; }
};
struct Derived : Base {
  int id() const override { return 2; }
};

struct Node {
  static int alive;
  SharedPtr<Node> child;  // strong down-link
  WeakPtr<Node> parent;   // weak up-link: breaks the cycle
  Node() { ++alive; }
  ~Node() { --alive; }
};
int Node::alive = 0;

struct Pair {
  int a = 0;
  int b = 0;
};

void reset_counters() {
  Tracked::alive = 0;
  Tracked::dtors = 0;
  CountingDeleter::calls = 0;
  Node::alive = 0;
}

}  // namespace

TEST(SharedPtrTest, EmptyStateIsDefined) {
  SharedPtr<int> s;
  EXPECT_EQ(s.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(s));
  EXPECT_EQ(s.use_count(), 0);
  EXPECT_FALSE(s.unique());
  EXPECT_EQ(s, nullptr);

  WeakPtr<int> w;
  EXPECT_EQ(w.use_count(), 0);
  EXPECT_TRUE(w.expired());
  EXPECT_EQ(w.lock().get(), nullptr);
}

TEST(SharedPtrTest, TakesOwnership) {
  reset_counters();
  {
    SharedPtr<Tracked> s(new Tracked(5));
    ASSERT_NE(s.get(), nullptr) << "ctor must take ownership";
    EXPECT_TRUE(static_cast<bool>(s));
    EXPECT_EQ(s->value, 5);
    EXPECT_EQ((*s).value, 5);
    EXPECT_EQ(s.use_count(), 1);
    EXPECT_TRUE(s.unique());
    EXPECT_EQ(Tracked::alive, 1);
  }
  EXPECT_EQ(Tracked::alive, 0);
  EXPECT_EQ(Tracked::dtors, 1);
}

TEST(SharedPtrTest, CopySharesOwnership) {
  reset_counters();
  {
    SharedPtr<Tracked> a(new Tracked(1));
    SharedPtr<Tracked> b = a;
    EXPECT_EQ(a.use_count(), 2);
    EXPECT_EQ(b.use_count(), 2);
    EXPECT_EQ(a.get(), b.get());
    EXPECT_FALSE(a.unique());
    ASSERT_NE(b.get(), nullptr);
    EXPECT_EQ(b->value, 1);
    EXPECT_EQ(Tracked::alive, 1);  // one object, two owners
  }
  EXPECT_EQ(Tracked::alive, 0);  // destroyed exactly once
  EXPECT_EQ(Tracked::dtors, 1);
}

TEST(SharedPtrTest, MoveTransfersWithoutBumpingCount) {
  reset_counters();
  SharedPtr<Tracked> a(new Tracked(7));
  Tracked* raw = a.get();
  SharedPtr<Tracked> b(std::move(a));
  EXPECT_EQ(a.get(), nullptr);  // source emptied
  EXPECT_EQ(a.use_count(), 0);
  ASSERT_NE(b.get(), nullptr) << "move must transfer ownership";
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(b.use_count(), 1);  // no extra count: ownership moved, not shared
  EXPECT_EQ(b->value, 7);
  EXPECT_EQ(Tracked::alive, 1);
}

TEST(SharedPtrTest, CopyAssignReleasesOld) {
  reset_counters();
  SharedPtr<Tracked> a(new Tracked(1));
  SharedPtr<Tracked> b(new Tracked(2));
  a = b;
  EXPECT_EQ(Tracked::dtors, 1);  // a's old object released
  EXPECT_EQ(a.get(), b.get());
  EXPECT_EQ(a.use_count(), 2);
  ASSERT_NE(a.get(), nullptr);
  EXPECT_EQ(a->value, 2);
}

TEST(SharedPtrTest, MoveAssignReleasesOld) {
  reset_counters();
  SharedPtr<Tracked> a(new Tracked(1));
  SharedPtr<Tracked> b(new Tracked(2));
  Tracked* raw = b.get();
  a = std::move(b);
  EXPECT_EQ(b.get(), nullptr);
  EXPECT_EQ(a.get(), raw);
  EXPECT_EQ(Tracked::dtors, 1);  // a's old object released
  ASSERT_NE(a.get(), nullptr);
  EXPECT_EQ(a->value, 2);
  EXPECT_EQ(a.use_count(), 1);
}

TEST(SharedPtrTest, SelfAssignIsSafe) {
  SharedPtr<int> s(new int(4));
  int* raw = s.get();
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  s = s;             // self copy-assign keeps ownership
  s = std::move(s);  // self move-assign keeps ownership
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  EXPECT_EQ(s.get(), raw);
  ASSERT_NE(s.get(), nullptr);
  EXPECT_EQ(*s, 4);
}

TEST(SharedPtrTest, ResetReleasesLastOwner) {
  reset_counters();
  SharedPtr<Tracked> s(new Tracked(1));
  SharedPtr<Tracked> keep = s;
  s.reset();
  EXPECT_EQ(s.get(), nullptr);
  EXPECT_EQ(s.use_count(), 0);
  EXPECT_EQ(Tracked::alive, 1);  // keep still owns it
  keep.reset();
  EXPECT_EQ(Tracked::alive, 0);
  EXPECT_EQ(Tracked::dtors, 1);
}

TEST(SharedPtrTest, ResetAdoptsNewPointer) {
  reset_counters();
  SharedPtr<Tracked> s(new Tracked(1));
  s.reset(new Tracked(2));
  EXPECT_EQ(Tracked::dtors, 1);  // old destroyed
  ASSERT_NE(s.get(), nullptr);
  EXPECT_EQ(s->value, 2);
  EXPECT_EQ(s.use_count(), 1);
  EXPECT_EQ(Tracked::alive, 1);
}

TEST(SharedPtrTest, SwapExchangesOwnership) {
  SharedPtr<int> a(new int(1));
  SharedPtr<int> b(new int(2));
  int* ra = a.get();
  int* rb = b.get();
  a.swap(b);
  EXPECT_EQ(a.get(), rb);
  EXPECT_EQ(b.get(), ra);
  swap(a, b);
  EXPECT_EQ(a.get(), ra);
  EXPECT_EQ(b.get(), rb);
}

TEST(SharedPtrTest, CustomDeleterRunsOnLastRelease) {
  reset_counters();
  {
    SharedPtr<int> s(new int(9), CountingDeleter{});
    {
      SharedPtr<int> c = s;
      EXPECT_EQ(s.use_count(), 2);
      EXPECT_EQ(CountingDeleter::calls, 0);  // not last yet
    }
    EXPECT_EQ(s.use_count(), 1);
    EXPECT_EQ(CountingDeleter::calls, 0);
  }
  EXPECT_EQ(CountingDeleter::calls, 1);  // last release runs the deleter
}

TEST(SharedPtrTest, ResetWithDeleterAdopts) {
  reset_counters();
  SharedPtr<int> s;
  s.reset(new int(3), CountingDeleter{});
  ASSERT_NE(s.get(), nullptr);
  EXPECT_EQ(s.use_count(), 1);
  EXPECT_EQ(*s, 3);
  s.reset();
  EXPECT_EQ(CountingDeleter::calls, 1);
}

TEST(SharedPtrTest, MakeSharedBuildsOneObject) {
  reset_counters();
  {
    auto s = MakeShared<Tracked>(42);
    ASSERT_NE(s.get(), nullptr) << "MakeShared must own an object";
    EXPECT_EQ(s->value, 42);
    EXPECT_EQ(s.use_count(), 1);
    EXPECT_TRUE(s.unique());
    EXPECT_EQ(Tracked::alive, 1);
  }
  EXPECT_EQ(Tracked::alive, 0);
  EXPECT_EQ(Tracked::dtors, 1);
}

TEST(SharedPtrTest, DerivedToBaseCopy) {
  SharedPtr<Derived> d = MakeShared<Derived>();
  ASSERT_NE(d.get(), nullptr);
  Derived* raw = d.get();
  SharedPtr<Base> b = d;  // shared ownership across the hierarchy
  EXPECT_EQ(d.use_count(), 2);
  ASSERT_NE(b.get(), nullptr);
  EXPECT_EQ(b->id(), 2);  // virtual dispatch intact
  EXPECT_EQ(b.get(), static_cast<Base*>(raw));
}

TEST(SharedPtrTest, AliasingSharesOwnershipOfAnotherPointer) {
  auto owner = MakeShared<Pair>();
  ASSERT_NE(owner.get(), nullptr);
  owner->a = 1;
  owner->b = 2;
  SharedPtr<int> view(owner, &owner->a);  // member view, shared lifetime
  EXPECT_EQ(owner.use_count(), 2);
  EXPECT_EQ(view.use_count(), 2);
  ASSERT_NE(view.get(), nullptr);
  EXPECT_EQ(*view, 1);
  owner.reset();  // Pair survives through the view
  EXPECT_EQ(view.use_count(), 1);
  EXPECT_EQ(*view, 1);
}

TEST(SharedPtrTest, WeakLockUpgradesWhileAlive) {
  auto s = MakeShared<Tracked>(8);
  ASSERT_NE(s.get(), nullptr);
  WeakPtr<Tracked> w = s;
  EXPECT_EQ(w.use_count(), 1);
  EXPECT_FALSE(w.expired());
  {
    SharedPtr<Tracked> up = w.lock();
    ASSERT_NE(up.get(), nullptr) << "lock must succeed while alive";
    EXPECT_EQ(up.get(), s.get());
    EXPECT_EQ(s.use_count(), 2);
    EXPECT_EQ(up->value, 8);
  }
  EXPECT_EQ(s.use_count(), 1);  // upgrade released
}

TEST(SharedPtrTest, WeakExpiresAfterLastSharedGone) {
  WeakPtr<Tracked> w;
  {
    auto s = MakeShared<Tracked>(8);
    ASSERT_NE(s.get(), nullptr);
    w = s;
    EXPECT_FALSE(w.expired());
  }  // last SharedPtr gone
  EXPECT_TRUE(w.expired());
  EXPECT_EQ(w.use_count(), 0);
  EXPECT_EQ(w.lock().get(), nullptr);  // upgrade of dead object is empty
}

TEST(SharedPtrTest, WeakBreaksReferenceCycles) {
  reset_counters();
  {
    auto parent = MakeShared<Node>();
    auto child = MakeShared<Node>();
    ASSERT_NE(parent.get(), nullptr);
    ASSERT_NE(child.get(), nullptr) << "nodes must exist before linking";
    parent->child = child;   // strong down-link
    child->parent = parent;  // weak up-link: no cycle
    EXPECT_EQ(Node::alive, 2);
  }  // both released despite the links
  EXPECT_EQ(Node::alive, 0);
}

TEST(SharedPtrTest, ConcurrentCopiesKeepExactlyOneOwner) {
  reset_counters();
  auto s = MakeShared<Tracked>(7);
  ASSERT_NE(s.get(), nullptr);
  constexpr int kThreads = 4;
  constexpr int kCopies = 2000;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&] {
      for (int i = 0; i < kCopies; ++i) {
        SharedPtr<Tracked> c = s;  // atomic refcount bump
        if (c.get() == nullptr) {
          FAIL() << "copy must share ownership";
          return;
        }
        EXPECT_EQ(c->value, 7);
      }
    });
  }
  for (auto& th : threads) th.join();
  EXPECT_EQ(s.use_count(), 1);
  EXPECT_EQ(Tracked::alive, 1);
}

TEST(SharedPtrTest, EqualityComparesPointers) {
  auto a = MakeShared<int>(1);
  auto b = a;
  auto c = MakeShared<int>(1);
  ASSERT_NE(a.get(), nullptr);
  ASSERT_NE(c.get(), nullptr);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a != c);  // distinct objects, distinct pointers
  EXPECT_TRUE(a != nullptr);
  EXPECT_TRUE(nullptr == SharedPtr<int>());
}
