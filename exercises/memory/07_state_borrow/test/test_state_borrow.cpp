#include "state_borrow.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

// The static half of the rule, checked at compile time.
static_assert(std::is_copy_constructible_v<SRef<int>>);
static_assert(!std::is_copy_constructible_v<SMut<int>>);
static_assert(std::is_move_constructible_v<SMut<int>>);
static_assert(std::is_same_v<decltype(*std::declval<const SRef<int>&>()),
                             const int&>);
static_assert(std::is_same_v<decltype(*std::declval<const SMut<int>&>()),
                             int&>);

TEST(StateBorrowTest, StartsFree) {
  SBox<int> box(1);
  EXPECT_EQ(box.readers(), 0u);
}

TEST(StateBorrowTest, SharedBorrowsAlwaysSucceed) {
  SBox<int> box(41);
  auto a = box.borrow();
  auto b = box.borrow();
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value()) << "no writer can exist while the box is alive";
  EXPECT_EQ(box.readers(), 2u);
  EXPECT_EQ(**a, 41);
}

TEST(StateBorrowTest, SharedHandlesAreConstOnly) {
  SBox<std::string> box("feed");
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  const std::string& s = **r;  // compiles: const access...
  EXPECT_EQ(s, "feed");
  // (**r = "x") would not compile: operator* yields const T& (see asserts).
}

TEST(StateBorrowTest, ExclusiveMoveConsumesTheBox) {
  SBox<int> box(5);
  auto pair = std::move(box).borrow_mut();
  LockedBox<int>& locked = pair.first;
  SMut<int>& m = pair.second;
  ASSERT_TRUE(m);
  *m = 6;
  SBox<int> back = std::move(locked).release(std::move(m));
  EXPECT_EQ(back.readers(), 0u);
  auto r = back.borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(**r, 6) << "mutation through the exclusive handle persists";
}

TEST(StateBorrowTest, ExclusiveBlockedByLiveReaders) {
  SBox<int> box(5);
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_THROW(std::move(box).borrow_mut(), std::logic_error)
      << "readers must drain before the box can be consumed";
}

TEST(StateBorrowTest, ReleaseRestoresBorrowableBox) {
  SBox<int> box(1);
  {
    auto r = box.borrow();
    ASSERT_TRUE(r.has_value());
  }
  auto pair = std::move(box).borrow_mut();
  ASSERT_TRUE(pair.second);
  SBox<int> back = std::move(pair.first).release(std::move(pair.second));
  auto r1 = back.borrow();
  auto r2 = back.borrow();
  EXPECT_TRUE(r1.has_value());
  EXPECT_TRUE(r2.has_value()) << "released box borrows freely again";
  EXPECT_EQ(back.readers(), 2u);
}

TEST(StateBorrowTest, ReaderCountIsExact) {
  SBox<int> box(0);
  {
    auto a = box.borrow();
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(box.readers(), 1u);
    {
      SRef<int> b = *a;  // copies share the count
      EXPECT_EQ(box.readers(), 2u);
    }
    EXPECT_EQ(box.readers(), 1u);
  }
  EXPECT_EQ(box.readers(), 0u);
}
