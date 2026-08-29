#include <gtest/gtest.h>

#include "binary_search_tree.h"

#include <random>
#include <stdexcept>
#include <vector>

TEST(BinarySearchTreeTest, NewTreeIsEmpty) {
  BinarySearchTree t;
  EXPECT_TRUE(t.empty());
  EXPECT_EQ(t.size(), 0u);
  EXPECT_THROW(t.min(), std::out_of_range);
  EXPECT_THROW(t.max(), std::out_of_range);
  EXPECT_THROW(t.nearest(10), std::out_of_range);
  EXPECT_TRUE(t.in_order().empty());
}

TEST(BinarySearchTreeTest, InsertReturnsTrueOnlyForNewKeys) {
  BinarySearchTree t;
  EXPECT_TRUE(t.insert(5));
  EXPECT_EQ(t.size(), 1u);
  EXPECT_FALSE(t.insert(5));  // duplicate rejected
  EXPECT_EQ(t.size(), 1u);
  EXPECT_TRUE(t.insert(3));
  EXPECT_TRUE(t.insert(8));
  EXPECT_EQ(t.size(), 3u);
}

TEST(BinarySearchTreeTest, InOrderIsSortedRegardlessOfInsertionOrder) {
  BinarySearchTree t;
  for (int k : {50, 30, 70, 20, 40, 60, 80}) t.insert(k);
  EXPECT_EQ(t.in_order(), (std::vector<int>{20, 30, 40, 50, 60, 70, 80}));
}

TEST(BinarySearchTreeTest, ContainsAfterInsertsAndErases) {
  BinarySearchTree t;
  for (int k : {9, 4, 7, 1, 8}) t.insert(k);
  EXPECT_TRUE(t.contains(7));
  EXPECT_TRUE(t.contains(1));
  EXPECT_FALSE(t.contains(99));
  EXPECT_TRUE(t.erase(7));
  EXPECT_FALSE(t.contains(7));
  EXPECT_FALSE(t.erase(7));  // already gone
  EXPECT_EQ(t.size(), 4u);
}

TEST(BinarySearchTreeTest, EraseLeaf) {
  BinarySearchTree t;
  for (int k : {5, 3, 8}) t.insert(k);
  EXPECT_TRUE(t.erase(3));  // leaf
  EXPECT_EQ(t.in_order(), (std::vector<int>{5, 8}));
  EXPECT_EQ(t.size(), 2u);
}

TEST(BinarySearchTreeTest, EraseOneChild) {
  BinarySearchTree t;
  for (int k : {5, 3, 2}) t.insert(k);  // 3 has only a left child
  EXPECT_TRUE(t.erase(3));
  EXPECT_EQ(t.in_order(), (std::vector<int>{2, 5}));

  BinarySearchTree u;
  for (int k : {5, 3, 9, 15}) u.insert(k);  // 9 has only a right child
  EXPECT_TRUE(u.erase(9));
  EXPECT_EQ(u.in_order(), (std::vector<int>{3, 5, 15}));
}

TEST(BinarySearchTreeTest, EraseTwoChildrenUsesSuccessor) {
  BinarySearchTree t;
  for (int k : {50, 30, 70, 20, 40, 60, 80}) t.insert(k);
  EXPECT_TRUE(t.erase(50));  // root with two children, successor = 60
  EXPECT_EQ(t.in_order(), (std::vector<int>{20, 30, 40, 60, 70, 80}));
  EXPECT_EQ(t.size(), 6u);
}

TEST(BinarySearchTreeTest, EraseTwoChildrenSuccessorIsRightChild) {
  BinarySearchTree t;
  for (int k : {50, 30, 55, 52}) t.insert(k);  // successor of 50 is 52's... 52
  // erase 55 (two children: 52 left only) -> successor is right child 52
  EXPECT_TRUE(t.erase(55));
  EXPECT_FALSE(t.contains(55));
  EXPECT_TRUE(t.contains(52));
  EXPECT_EQ(t.in_order(), (std::vector<int>{30, 50, 52}));
}

TEST(BinarySearchTreeTest, MinMax) {
  BinarySearchTree t;
  for (int k : {40, 10, 90, 25, 70}) t.insert(k);
  EXPECT_EQ(t.min(), 10);
  EXPECT_EQ(t.max(), 90);
  t.erase(10);
  EXPECT_EQ(t.min(), 25);
  t.erase(90);
  EXPECT_EQ(t.max(), 70);
}

TEST(BinarySearchTreeTest, NearestPicksClosestAndTiesGoSmaller) {
  BinarySearchTree even;
  even.insert(20);
  even.insert(30);
  EXPECT_EQ(even.nearest(25), 20);  // equidistant -> smaller
  EXPECT_EQ(even.nearest(21), 20);
  EXPECT_EQ(even.nearest(29), 30);
  EXPECT_EQ(even.nearest(100), 30);  // beyond max
  EXPECT_EQ(even.nearest(0), 20);    // below min

  BinarySearchTree t;
  for (int k : {10, 40, 5}) t.insert(k);
  EXPECT_EQ(t.nearest(8), 10);  // |10-8|=2 beats |5-8|=3
  EXPECT_EQ(t.nearest(12), 10);
  EXPECT_EQ(t.nearest(6), 5);
  EXPECT_EQ(t.nearest(10), 10);  // exact hit
}

TEST(BinarySearchTreeTest, CopyIsDeepAndAssignReplaces) {
  BinarySearchTree t;
  for (int k : {30, 10, 50}) t.insert(k);
  BinarySearchTree copy(t);
  copy.insert(99);
  copy.erase(10);
  EXPECT_EQ(t.in_order(), (std::vector<int>{10, 30, 50}));
  EXPECT_EQ(copy.in_order(), (std::vector<int>{30, 50, 99}));

  BinarySearchTree assigned;
  assigned = t;
  EXPECT_EQ(assigned.in_order(), (std::vector<int>{10, 30, 50}));
  t.insert(5);
  EXPECT_EQ(assigned.in_order(), (std::vector<int>{10, 30, 50}));
}

TEST(BinarySearchTreeTest, RandomShuffleMaintainsOrder) {
  std::vector<int> keys(100);
  for (int i = 0; i < 100; ++i) keys[i] = i;
  std::mt19937 rng(999);
  std::shuffle(keys.begin(), keys.end(), rng);
  BinarySearchTree t;
  for (int k : keys) t.insert(k);
  EXPECT_EQ(t.size(), 100u);
  auto sorted = t.in_order();
  ASSERT_EQ(sorted.size(), 100u);
  for (int i = 0; i < 100; ++i) EXPECT_EQ(sorted[i], i);  // == keys sorted

  for (int i = 0; i < 100; i += 2) EXPECT_TRUE(t.erase(i));
  EXPECT_EQ(t.size(), 50u);
  auto now = t.in_order();
  for (int k : now) EXPECT_EQ(k % 2, 1);  // only odd keys remain
}

TEST(BinarySearchTreeTest, EraseDownToEmpty) {
  BinarySearchTree t;
  for (int k : {5, 3, 8, 1, 4}) t.insert(k);
  const std::vector<int> all = t.in_order();
  for (int k : all) EXPECT_TRUE(t.erase(k));
  EXPECT_TRUE(t.empty());
  EXPECT_EQ(t.size(), 0u);
  EXPECT_TRUE(t.in_order().empty());
  EXPECT_TRUE(t.insert(42));  // reusable after emptying
  EXPECT_EQ(t.in_order(), (std::vector<int>{42}));
}