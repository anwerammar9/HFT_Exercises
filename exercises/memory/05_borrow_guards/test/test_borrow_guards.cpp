#include "borrow_guards.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

TEST(BorrowGuardsTest, StartsFree) {
  BorrowBox<int> box(1);
  EXPECT_EQ(box.readers(), 0u);
  EXPECT_FALSE(box.is_writing());
}

TEST(BorrowGuardsTest, SharedBorrowReads) {
  BorrowBox<int> box(41);
  auto r = box.try_borrow();
  ASSERT_TRUE(r.has_value()) << "first shared borrow must succeed";
  EXPECT_EQ(**r, 41);
  EXPECT_EQ(r->get(), &**r);
  EXPECT_EQ(box.readers(), 1u);
  EXPECT_FALSE(box.is_writing());
}

TEST(BorrowGuardsTest, ManySharedBorrowsCoexist) {
  BorrowBox<std::string> box("tick");
  auto a = box.try_borrow();
  auto b = box.try_borrow();
  auto c = box.try_borrow();
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());
  ASSERT_TRUE(c.has_value()) << "shared borrows are unlimited";
  EXPECT_EQ(box.readers(), 3u);
  EXPECT_EQ(**a, "tick");
  EXPECT_EQ(**c, "tick");
}

TEST(BorrowGuardsTest, GuardReleaseRestoresFreeState) {
  BorrowBox<int> box(2);
  {
    auto a = box.try_borrow();
    auto b = box.try_borrow();
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(box.readers(), 2u);
  }  // both guards drop here
  EXPECT_EQ(box.readers(), 0u);
  EXPECT_FALSE(box.is_writing());
}

TEST(BorrowGuardsTest, MutableBorrowExcludesEverything) {
  BorrowBox<int> box(5);
  auto w = box.try_borrow_mut();
  ASSERT_TRUE(w.has_value()) << "free box must grant mut borrow";
  EXPECT_TRUE(box.is_writing());
  EXPECT_FALSE(box.try_borrow().has_value()) << "no shared while writing";
  EXPECT_FALSE(box.try_borrow_mut().has_value()) << "no second writer";
}

TEST(BorrowGuardsTest, SharedBorrowBlocksMutable) {
  BorrowBox<int> box(5);
  auto r = box.try_borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_FALSE(box.try_borrow_mut().has_value())
      << "mut borrow must fail while readers live";
  r.reset();  // release the reader...
  auto w = box.try_borrow_mut();
  EXPECT_TRUE(w.has_value()) << "mut borrow succeeds once readers drain";
}

TEST(BorrowGuardsTest, MutationThroughGuardIsVisible) {
  BorrowBox<int> box(1);
  {
    auto w = box.try_borrow_mut();
    ASSERT_TRUE(w.has_value());
    **w = 99;
    EXPECT_EQ(w->get(), &**w);
  }
  auto r = box.try_borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(**r, 99);
}

TEST(BorrowGuardsTest, MovedGuardKeepsBorrowAlive) {
  BorrowBox<int> box(3);
  auto a = box.try_borrow();
  ASSERT_TRUE(a.has_value());
  std::optional<BorrowRef<int>> b = std::move(a);
  ASSERT_TRUE(a.has_value());  // the optional shell stays...
  EXPECT_FALSE(static_cast<bool>(*a)) << "moved-from guard is empty";
  ASSERT_TRUE(b.has_value());
  EXPECT_EQ(box.readers(), 1u) << "the borrow survives the move";
  b.reset();
  EXPECT_EQ(box.readers(), 0u);
}

TEST(BorrowGuardsTest, MovedMutGuardKeepsExclusion) {
  BorrowBox<int> box(3);
  auto a = box.try_borrow_mut();
  ASSERT_TRUE(a.has_value());
  std::optional<BorrowMut<int>> b = std::move(a);
  ASSERT_TRUE(b.has_value());
  EXPECT_TRUE(box.is_writing());
  EXPECT_FALSE(box.try_borrow().has_value());
  b.reset();
  EXPECT_FALSE(box.is_writing());
}

TEST(BorrowGuardsTest, InterleavedBorrowCycle) {
  BorrowBox<int> box(0);
  for (int i = 0; i < 10; ++i) {
    {
      auto r1 = box.try_borrow();
      auto r2 = box.try_borrow();
      ASSERT_TRUE(r1.has_value());
      ASSERT_TRUE(r2.has_value());
      EXPECT_EQ(**r1, i);
    }
    {
      auto w = box.try_borrow_mut();
      ASSERT_TRUE(w.has_value()) << "readers drained, write must proceed";
      **w = i + 1;
    }
  }
  auto r = box.try_borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(**r, 10);
}
