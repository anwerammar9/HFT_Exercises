#include "unique_ptr.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <type_traits>
#include <utility>

namespace {

struct Tracked {
  static int alive;
  static int dtors;
  int value = 0;
  explicit Tracked(int v = 0) : value(v) { ++alive; }
  Tracked(const Tracked& o) : value(o.value) { ++alive; }
  Tracked& operator=(const Tracked& o) {
    value = o.value;
    return *this;
  }
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

struct FreeDeleter {
  void operator()(int* p) const noexcept { std::free(p); }
};

struct Base {
  virtual ~Base() = default;
  virtual int id() const { return 1; }
};
struct Derived : Base {
  int id() const override { return 2; }
};

void reset_counters() {
  Tracked::alive = 0;
  Tracked::dtors = 0;
  CountingDeleter::calls = 0;
}

}  // namespace

static_assert(!std::is_copy_constructible_v<UniquePtr<int>>);
static_assert(!std::is_copy_assignable_v<UniquePtr<int>>);
static_assert(std::is_move_constructible_v<UniquePtr<int>>);
static_assert(std::is_move_assignable_v<UniquePtr<int>>);

TEST(UniquePtrTest, DefaultIsNull) {
  UniquePtr<int> p;
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_EQ(p, nullptr);
  EXPECT_EQ(nullptr, p);
}

TEST(UniquePtrTest, TakesOwnershipAndDestroys) {
  reset_counters();
  {
    UniquePtr<Tracked> p(new Tracked(42));
    ASSERT_NE(p.get(), nullptr);
    EXPECT_TRUE(static_cast<bool>(p));
    EXPECT_EQ(p->value, 42);
    EXPECT_EQ((*p).value, 42);
    EXPECT_EQ(Tracked::alive, 1);
  }
  EXPECT_EQ(Tracked::alive, 0);
  EXPECT_EQ(Tracked::dtors, 1);
}

TEST(UniquePtrTest, MoveCtorTransfersOwnership) {
  reset_counters();
  UniquePtr<Tracked> a(new Tracked(7));
  Tracked* raw = a.get();
  UniquePtr<Tracked> b(std::move(a));
  EXPECT_EQ(a.get(), nullptr);  // source emptied
  ASSERT_NE(b.get(), nullptr) << "move must transfer ownership to b";
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(b->value, 7);
  EXPECT_EQ(Tracked::alive, 1);  // no double ownership
}

TEST(UniquePtrTest, MoveAssignDestroysOldAndTransfers) {
  reset_counters();
  UniquePtr<Tracked> a(new Tracked(1));
  UniquePtr<Tracked> b(new Tracked(2));
  b = std::move(a);
  EXPECT_EQ(a.get(), nullptr);
  ASSERT_NE(b.get(), nullptr);
  EXPECT_EQ(b->value, 1);
  EXPECT_EQ(Tracked::dtors, 1);  // b's old object destroyed
  EXPECT_EQ(Tracked::alive, 1);
}

TEST(UniquePtrTest, SelfMoveAssignIsSafeNoOp) {
  UniquePtr<int> p(new int(9));
  int* raw = p.get();
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  p = std::move(p);  // deliberate self-move
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  EXPECT_EQ(p.get(), raw);
  EXPECT_EQ(*p, 9);
}

TEST(UniquePtrTest, ReleaseRelinquishesWithoutDestroying) {
  reset_counters();
  UniquePtr<Tracked> p(new Tracked(3));
  Tracked* raw = p.release();
  ASSERT_NE(raw, nullptr);
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_EQ(Tracked::alive, 1);  // still alive: caller owns it now
  EXPECT_EQ(raw->value, 3);
  delete raw;
  EXPECT_EQ(Tracked::alive, 0);
}

TEST(UniquePtrTest, ResetReplacesAndDestroysOld) {
  reset_counters();
  UniquePtr<Tracked> p(new Tracked(1));
  p.reset(new Tracked(2));
  EXPECT_EQ(Tracked::dtors, 1);  // old destroyed
  ASSERT_NE(p.get(), nullptr);
  EXPECT_EQ(p->value, 2);
  EXPECT_EQ(Tracked::alive, 1);
}

TEST(UniquePtrTest, ResetToNullDeletes) {
  reset_counters();
  UniquePtr<Tracked> p(new Tracked(1));
  p.reset();
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_EQ(Tracked::alive, 0);
  EXPECT_EQ(Tracked::dtors, 1);
  p.reset(nullptr);  // resetting null is a safe no-op
  EXPECT_EQ(p, nullptr);
}

TEST(UniquePtrTest, SwapExchangesOwnership) {
  UniquePtr<int> a(new int(1));
  UniquePtr<int> b(new int(2));
  int* ra = a.get();
  int* rb = b.get();
  a.swap(b);
  EXPECT_EQ(a.get(), rb);
  EXPECT_EQ(b.get(), ra);
  swap(a, b);  // free-function swap round-trips
  EXPECT_EQ(a.get(), ra);
  EXPECT_EQ(b.get(), rb);
}

TEST(UniquePtrTest, CustomDeleterIsInvoked) {
  reset_counters();
  {
    UniquePtr<int, CountingDeleter> p(new int(5), CountingDeleter{});
    EXPECT_EQ(*p, 5);
    EXPECT_EQ(CountingDeleter::calls, 0);
  }
  EXPECT_EQ(CountingDeleter::calls, 1);
}

TEST(UniquePtrTest, CustomDeleterWithMallocFree) {
  auto* raw = static_cast<int*>(std::malloc(sizeof(int)));
  ASSERT_NE(raw, nullptr);
  *raw = 11;
  {
    UniquePtr<int, FreeDeleter> p(raw);
    EXPECT_EQ(*p, 11);
  }  // free(), not delete: clean under ASan
}

TEST(UniquePtrTest, DerivedToBaseMove) {
  UniquePtr<Derived> d(new Derived());
  Derived* raw = d.get();
  UniquePtr<Base> b(std::move(d));
  EXPECT_EQ(d.get(), nullptr);
  ASSERT_NE(b.get(), nullptr);
  EXPECT_EQ(b->id(), 2);  // virtual dispatch survives the move
  EXPECT_EQ(b.get(), static_cast<Base*>(raw));
}

TEST(UniquePtrTest, ArrayIndexingAndArrayDelete) {
  reset_counters();
  {
    UniquePtr<Tracked[]> p(new Tracked[3]);
    EXPECT_EQ(Tracked::alive, 3);
    p[0].value = 10;
    p[2].value = 30;
    EXPECT_EQ(p[0].value, 10);
    EXPECT_EQ(p[2].value, 30);
  }
  EXPECT_EQ(Tracked::alive, 0);  // delete[] ran all 3 dtors
  EXPECT_EQ(Tracked::dtors, 3);
}

TEST(UniquePtrTest, MakeUniqueFactory) {
  reset_counters();
  auto p = MakeUnique<Tracked>(99);
  ASSERT_NE(p.get(), nullptr);
  EXPECT_EQ(p->value, 99);
  EXPECT_EQ(Tracked::alive, 1);
}

TEST(UniquePtrTest, MakeUniqueArrayFactories) {
  auto z = MakeUniqueArray<int>(4);
  ASSERT_NE(z.get(), nullptr);
  for (int i = 0; i < 4; ++i) EXPECT_EQ(z[i], 0);  // value-initialized

  auto f = MakeUniqueArray<int>(3, 7);
  ASSERT_NE(f.get(), nullptr);
  for (int i = 0; i < 3; ++i) EXPECT_EQ(f[i], 7);
}
