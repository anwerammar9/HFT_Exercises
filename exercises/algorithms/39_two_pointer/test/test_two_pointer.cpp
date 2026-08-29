#include <gtest/gtest.h>

#include "two_pointer.h"

#include <string>
#include <vector>

namespace {

using two_pointer::has_sum_pair;
using two_pointer::has_triple_sum;
using two_pointer::is_palindrome;
using two_pointer::max_area;
using two_pointer::merge_sorted;
using two_pointer::remove_duplicates;

}  // namespace

TEST(TwoPointerTest, SumPairFindsOppositeEnds) {
  EXPECT_TRUE(has_sum_pair({1, 2, 3, 4, 5}, 7));    // 2+5
  EXPECT_TRUE(has_sum_pair({1, 2, 3, 4, 5}, 4));    // 1+3
  EXPECT_TRUE(has_sum_pair({-3, -1, 0, 2, 5}, 4));  // -1+5
}

TEST(TwoPointerTest, SumPairCannotReuseOneSlot) {
  EXPECT_FALSE(has_sum_pair({1, 2, 3, 4, 5}, 10));  // would need 5+5
  EXPECT_FALSE(has_sum_pair({4}, 8));
  EXPECT_FALSE(has_sum_pair({}, 0));
}

TEST(TwoPointerTest, SumPairAcrossTwoArrays) {
  EXPECT_TRUE(has_sum_pair({1, 2, 3}, {10, 20, 30}, 22));
  EXPECT_TRUE(has_sum_pair({1, 2, 3}, {10, 20, 30}, 33));
  EXPECT_FALSE(has_sum_pair({1, 2, 3}, {10, 20, 30}, 5));   // both from `a`
  EXPECT_FALSE(has_sum_pair({}, {10, 20, 30}, 30));
  EXPECT_FALSE(has_sum_pair({1, 2, 3}, {}, 3));
}

TEST(TwoPointerTest, MergeSortedCombinesInterleaved) {
  EXPECT_EQ(merge_sorted({1, 3, 5}, {2, 4, 6}), (std::vector<int>{1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(merge_sorted({}, {1, 2}), (std::vector<int>{1, 2}));
  EXPECT_EQ(merge_sorted({1, 2}, {}), (std::vector<int>{1, 2}));
  EXPECT_EQ(merge_sorted({}, {}), (std::vector<int>{}));
  EXPECT_EQ(merge_sorted({1, 2, 2, 3}, {2, 4}),
            (std::vector<int>{1, 2, 2, 2, 3, 4}));
  EXPECT_EQ(merge_sorted({1, 1, 1}, {-1, 0}),
            (std::vector<int>{-1, 0, 1, 1, 1}));
}

TEST(TwoPointerTest, RemoveDuplicatesInPlace) {
  std::vector<int> v{1, 1, 2, 3, 3, 3, 4};
  const std::size_t n = remove_duplicates(v);
  EXPECT_EQ(n, 4u);
  EXPECT_EQ(std::vector<int>(v.begin(), v.begin() + n),
            (std::vector<int>{1, 2, 3, 4}));

  std::vector<int> all_same{7, 7, 7};
  EXPECT_EQ(remove_duplicates(all_same), 1u);

  std::vector<int> distinct{1, 2, 3};
  EXPECT_EQ(remove_duplicates(distinct), 3u);

  std::vector<int> empty;
  EXPECT_EQ(remove_duplicates(empty), 0u);
}

TEST(TwoPointerTest, TripleSumDetectsAndRejects) {
  EXPECT_TRUE(has_triple_sum({-4, -1, -1, 0, 1, 2}, 0));  // -1 + -1 + 2
  EXPECT_TRUE(has_triple_sum({1, 2, 3, 4}, 9));           // 2+3+4
  EXPECT_FALSE(has_triple_sum({1, 2, 3}, 7));
  EXPECT_FALSE(has_triple_sum({}, 0));
  EXPECT_FALSE(has_triple_sum({1, 2}, 3));  // needs three distinct slots
}

TEST(TwoPointerTest, MaxAreaClassicAndEdges) {
  EXPECT_EQ(max_area({1, 8, 6, 2, 5, 4, 8, 3, 7}), 49);
  EXPECT_EQ(max_area({1, 1}), 1);
  EXPECT_EQ(max_area({0, 0}), 0);
  EXPECT_EQ(max_area({1, 2, 3, 4}), 4);
  EXPECT_EQ(max_area({4, 3, 2, 1}), 4);
  EXPECT_EQ(max_area({}), 0);
  EXPECT_EQ(max_area({5}), 0);
}

TEST(TwoPointerTest, PalindromeStrict) {
  EXPECT_TRUE(is_palindrome("racecar"));
  EXPECT_TRUE(is_palindrome("abba"));
  EXPECT_TRUE(is_palindrome("a"));
  EXPECT_TRUE(is_palindrome(""));
  EXPECT_FALSE(is_palindrome("abc"));
  EXPECT_FALSE(is_palindrome("Racecar"));   // case matters
  EXPECT_FALSE(is_palindrome("a man"));     // space counts
  EXPECT_FALSE(is_palindrome("ab"));
}