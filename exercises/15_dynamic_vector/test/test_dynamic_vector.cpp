#include <gtest/gtest.h>

#include "dynamic_vector.h"

#include <numeric>
#include <stdexcept>
#include <string>

namespace {

// Records how many copies/moves a real (not stubbed) Vector performs, so the
// growth test can tell moving-reallocations from copying-reallocations.
struct Tracked {
  static int copies;
  static int moves;
  int v;
  explicit Tracked(int x) : v(x) {}
  Tracked(const Tracked& o) : v(o.v) { ++copies; }
  Tracked(Tracked&& o) noexcept : v(o.v) { ++moves; }
  Tracked& operator=(const Tracked&) = default;
  Tracked& operator=(Tracked&&) = default;
};
int Tracked::copies = 0;
int Tracked::moves = 0;

}  // namespace

TEST(VectorTest, NewVectorIsEmpty) {
  Vector<int> v;
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.size(), 0u);
  EXPECT_EQ(v.capacity(), 0u);
  EXPECT_EQ(v.data(), nullptr);
  EXPECT_EQ(v.begin(), v.end());
}

TEST(VectorTest, PushBackGrowsAndReadsBack) {
  Vector<int> v;
  v.push_back(10);
  v.push_back(20);
  v.push_back(30);
  ASSERT_EQ(v.size(), 3u);
  ASSERT_GE(v.capacity(), v.size());
  EXPECT_EQ(v[0], 10);
  EXPECT_EQ(v[1], 20);
  EXPECT_EQ(v[2], 30);
  EXPECT_EQ(v.front(), 10);
  EXPECT_EQ(v.back(), 30);
}

TEST(VectorTest, EmplaceBackConstructsInPlace) {
  Vector<std::string> v;
  v.emplace_back(3, 'x');
  ASSERT_EQ(v.size(), 1u);
  EXPECT_EQ(v[0], "xxx");

  // Move-only element type via emplace: raw-pointer arg is the interesting case.
  Vector<std::unique_ptr<int>> u;
  auto& ref = u.emplace_back(new int(42));
  ASSERT_EQ(u.size(), 1u);
  EXPECT_EQ(*ref, 42);
}

TEST(VectorTest, AtThrowsOutOfRange) {
  Vector<int> v;
  v.push_back(1);
  ASSERT_EQ(v.size(), 1u);
  EXPECT_EQ(v.at(0), 1);
  EXPECT_THROW(v.at(1), std::out_of_range);
}

TEST(VectorTest, ReservePreallocatesAndPreservesElements) {
  Vector<int> v;
  for (int i = 0; i < 32; ++i) v.push_back(i);
  ASSERT_EQ(v.size(), 32u);

  v.reserve(128);
  ASSERT_EQ(v.size(), 32u);
  ASSERT_GE(v.capacity(), 128u);
  for (int i = 0; i < 32; ++i) EXPECT_EQ(v[i], i);

  v.reserve(16);  // shrink request is a no-op
  ASSERT_GE(v.capacity(), 128u);
}

TEST(VectorTest, ShrinkToFitIsExactWhenAllocationsAllowed) {
  Vector<int> v;
  for (int i = 0; i < 64; ++i) v.push_back(i);
  ASSERT_EQ(v.size(), 64u);
  ASSERT_GE(v.capacity(), 64u);

  v.shrink_to_fit();
  ASSERT_EQ(v.capacity(), 64u);
  ASSERT_EQ(v.size(), 64u);
  for (int i = 0; i < 64; ++i) EXPECT_EQ(v[i], i);
}

TEST(VectorTest, GrowthPolicyStaysFactorTwo) {
  Vector<int> v;
  for (int i = 0; i < 65; ++i) v.push_back(i);
  ASSERT_EQ(v.size(), 65u);
  // Geometric growth keeps capacity between size and 2*size.
  ASSERT_GE(v.capacity(), v.size());
  ASSERT_LE(v.capacity(), 2 * v.size());
}

TEST(VectorTest, PopBackAndClear) {
  Vector<int> v;
  for (int i = 0; i < 5; ++i) v.push_back(i);
  ASSERT_EQ(v.size(), 5u);

  v.pop_back();
  v.pop_back();
  ASSERT_EQ(v.size(), 3u);
  EXPECT_EQ(v.back(), 2);

  v.clear();
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.size(), 0u);
  EXPECT_GE(v.capacity(), 3u);  // capacity is retained after clear
}

TEST(VectorTest, CopyAndMoveAreIndependent) {
  Vector<int> v;
  for (int i = 0; i < 10; ++i) v.push_back(i);
  ASSERT_EQ(v.size(), 10u);

  Vector<int> copy(v);
  ASSERT_EQ(copy.size(), v.size());
  ASSERT_GE(copy.capacity(), v.size());
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(copy[i], i);
    EXPECT_NE(&copy[i], &v[i]);  // distinct storage
  }

  Vector<int> moved(std::move(copy));
  ASSERT_EQ(moved.size(), 10u);
  for (int i = 0; i < 10; ++i) EXPECT_EQ(moved[i], i);
  EXPECT_EQ(copy.size(), 0u);  // moved-from is empty
}

TEST(VectorTest, IteratorsCoverTheRange) {
  Vector<int> v;
  for (int i = 1; i <= 100; ++i) v.push_back(i);
  ASSERT_EQ(v.size(), 100u);
  EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 5050);
  EXPECT_EQ(v.end() - v.begin(), 100);  // contiguous
}

TEST(VectorTest, MoveConstructionDoesNotCopyElements) {
  Vector<Tracked> v;
  for (int i = 0; i < 20; ++i) v.emplace_back(i);
  ASSERT_EQ(v.size(), 20u);
  Tracked::copies = Tracked::moves = 0;

  v.reserve(64);  // reallocation: should move, never copy
  ASSERT_EQ(Tracked::copies, 0);
  EXPECT_GT(Tracked::moves, 0);
}