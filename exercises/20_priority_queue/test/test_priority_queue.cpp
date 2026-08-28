#include <gtest/gtest.h>

#include "priority_queue.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>

namespace {

// A reorderable payload so tests can verify a max-by-(priority, seq) heap
// without touching tie ordering within a single priority.
struct Job {
  int priority;
  int seq;
  friend bool operator<(const Job& a, const Job& b) {
    return std::tie(a.priority, a.seq) < std::tie(b.priority, b.seq);
  }
};

}  // namespace

TEST(PriorityQueueTest, NewQueueIsEmpty) {
  PriorityQueue<int> pq;
  EXPECT_TRUE(pq.empty());
  EXPECT_EQ(pq.size(), 0u);
  EXPECT_THROW(pq.top(), std::out_of_range);
}

TEST(PriorityQueueTest, MaxHeapKeepsLargestOnTop) {
  PriorityQueue<int> pq;
  pq.push(3);
  pq.push(7);
  pq.push(1);
  pq.push(5);
  ASSERT_EQ(pq.size(), 4u);
  EXPECT_EQ(pq.top(), 7);
}

TEST(PriorityQueueTest, PopYieldsDescendingOrder) {
  PriorityQueue<int> pq;
  std::vector<int> in(64);
  std::iota(in.begin(), in.end(), 0);
  std::mt19937 rng(12345);
  std::shuffle(in.begin(), in.end(), rng);
  for (int x : in) pq.push(x);
  ASSERT_EQ(pq.size(), 64u);

  int prev = 999;
  for (int i = 0; i < 64; ++i) {
    EXPECT_LE(pq.top(), prev);  // strictly non-increasing
    prev = pq.top();
    pq.pop();
  }
  EXPECT_TRUE(pq.empty());
  EXPECT_EQ(pq.size(), 0u);
}

TEST(PriorityQueueTest, EqualPrioritiesAllEmerge) {
  PriorityQueue<int> pq;
  for (int i = 0; i < 8; ++i) pq.push(5);
  ASSERT_EQ(pq.size(), 8u);
  EXPECT_EQ(pq.top(), 5);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
  }
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueTest, CustomComparatorYieldsMinHeap) {
  PriorityQueue<int, std::greater<int>> pq;
  pq.push(3);
  pq.push(7);
  pq.push(1);
  pq.push(5);
  ASSERT_EQ(pq.size(), 4u);
  EXPECT_EQ(pq.top(), 1);
}

TEST(PriorityQueueTest, StructPayloadOrderedByKey) {
  PriorityQueue<Job> pq;
  pq.push({.priority = 1, .seq = 1});
  pq.push({.priority = 9, .seq = 2});
  pq.push({.priority = 5, .seq = 3});
  ASSERT_EQ(pq.size(), 3u);
  EXPECT_EQ(pq.top().priority, 9);
}

TEST(PriorityQueueTest, EmplaceConstructsInPlace) {
  PriorityQueue<std::string> pq;
  pq.emplace(3, 'x');  // "xxx" > "bb" > "a"
  pq.emplace(2, 'b');  // "bb"
  pq.emplace(1, 'a');  // "a"
  ASSERT_EQ(pq.size(), 3u);
  EXPECT_EQ(pq.top(), "xxx");
}

TEST(PriorityQueueTest, ClearDropsContents) {
  PriorityQueue<int> pq;
  pq.push(9);
  pq.push(2);
  ASSERT_EQ(pq.size(), 2u);
  pq.clear();
  EXPECT_TRUE(pq.empty());
  EXPECT_THROW(pq.top(), std::out_of_range);
}

TEST(PriorityQueueTest, InterleavedPushPopKeepsInvariant) {
  PriorityQueue<int> pq;
  for (int x : {4, 9, 1, 7, 3, 8, 2, 6, 5, 0}) pq.push(x);
  ASSERT_EQ(pq.size(), 10u);

  EXPECT_EQ(pq.top(), 9);
  pq.pop();
  EXPECT_EQ(pq.top(), 8);
  pq.push(11);
  pq.pop();  // removes 11
  pq.pop();  // removes 8
  EXPECT_EQ(pq.top(), 7);
  EXPECT_EQ(pq.size(), 8u);
}