#include <gtest/gtest.h>

#include "heap.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

struct Job {
  int priority;
  int seq;
  friend bool operator<(const Job& a, const Job& b) {
    return std::tie(a.priority, a.seq) < std::tie(b.priority, b.seq);
  }
};

std::vector<int> drain(BinaryHeap<int>& h) {
  std::vector<int> out;
  while (!h.empty()) {
    out.push_back(h.top());
    h.pop();
  }
  return out;
}

}  // namespace

TEST(HeapTest, NewHeapIsEmpty) {
  BinaryHeap<int> h;
  EXPECT_TRUE(h.empty());
  EXPECT_EQ(h.size(), 0u);
  EXPECT_THROW(h.top(), std::out_of_range);
  EXPECT_THROW(h.pop(), std::out_of_range);
}

TEST(HeapTest, InternalArrayIsHeapOrdered) {
  BinaryHeap<int> h;
  for (int x : {9, 4, 7, 1, 8, 2, 5, 3, 6, 0}) h.push(x);
  const auto& a = h.array();
  ASSERT_EQ(a.size(), 10u);
  EXPECT_TRUE(std::is_heap(a.begin(), a.end()));
}

TEST(HeapTest, PushPopYieldsDescendingOrder) {
  BinaryHeap<int> h;
  std::vector<int> in(64);
  std::iota(in.begin(), in.end(), 0);
  std::mt19937 rng(2024);
  std::shuffle(in.begin(), in.end(), rng);
  for (int x : in) h.push(x);
  std::vector<int> got = drain(h);
  EXPECT_EQ(got.size(), 64u);
  for (std::size_t i = 1; i < got.size(); ++i) EXPECT_GE(got[i - 1], got[i]);
  EXPECT_EQ(std::set<int>(got.begin(), got.end()).size(), 64u);
}

TEST(HeapTest, BuildFromRangeIsHeapAndComplete) {
  std::vector<int> in(32);
  std::iota(in.begin(), in.end(), 0);
  std::mt19937 rng(777);
  std::shuffle(in.begin(), in.end(), rng);
  BinaryHeap<int> h(in.begin(), in.end());
  ASSERT_EQ(h.size(), 32u);
  EXPECT_TRUE(std::is_heap(h.array().begin(), h.array().end()));
  std::vector<int> got = drain(h);
  EXPECT_EQ(std::set<int>(got.begin(), got.end()).size(), 32u);  // no loss/dup
}

TEST(HeapTest, BuildFromEmptyAndSingleRanges) {
  std::vector<int> none;
  BinaryHeap<int> h0(none.begin(), none.end());
  EXPECT_TRUE(h0.empty());

  std::vector<int> one{42};
  BinaryHeap<int> h1(one.begin(), one.end());
  ASSERT_EQ(h1.size(), 1u);
  EXPECT_EQ(h1.top(), 42);
}

TEST(HeapTest, EraseAtMiddleRemovesOnlyThatElement) {
  BinaryHeap<int> h;
  for (int x : {5, 3, 8, 1, 9, 4, 7, 2, 6, 0}) h.push(x);
  const auto value_at_2 = h.array()[2];
  h.erase_at(2);
  ASSERT_EQ(h.size(), 9u);
  EXPECT_TRUE(std::is_heap(h.array().begin(), h.array().end()));
  std::vector<int> got = drain(h);
  EXPECT_TRUE(std::find(got.begin(), got.end(), value_at_2) == got.end());
  EXPECT_EQ(std::set<int>(got.begin(), got.end()).size(), 9u);
}

TEST(HeapTest, EraseAtThrowsOutOfRange) {
  BinaryHeap<int> h;
  h.push(1);
  EXPECT_THROW(h.erase_at(1), std::out_of_range);
  EXPECT_THROW(h.erase_at(999), std::out_of_range);
}

TEST(HeapTest, EraseAtLastElementIsNoOpBeyondSize) {
  BinaryHeap<int> h;
  for (int x : {4, 2, 7}) h.push(x);
  const auto original = h.array();
  h.erase_at(h.size() - 1);
  ASSERT_EQ(h.size(), 2u);
  EXPECT_TRUE(std::is_heap(h.array().begin(), h.array().end()));
  std::vector<int> got = drain(h);
  EXPECT_EQ(std::set<int>(got.begin(), got.end()).size(), 2u);
}

TEST(HeapTest, ReplacePutsNewValueInAndPopsOldTop) {
  BinaryHeap<int> h;
  for (int x : {2, 5, 1}) h.push(x);
  h.replace(3);           // pops 5, pushes 3 -> {3,2,1}
  ASSERT_EQ(h.top(), 3);
  h.replace(9);           // pops 3, pushes 9 -> 9 emerges
  ASSERT_EQ(h.top(), 9);
  EXPECT_TRUE(std::is_heap(h.array().begin(), h.array().end()));
}

TEST(HeapTest, ReplaceOnEmptyPushes) {
  BinaryHeap<int> h;
  h.replace(7);
  ASSERT_EQ(h.size(), 1u);
  EXPECT_EQ(h.top(), 7);
}

TEST(HeapTest, EmplaceConstructsInPlace) {
  BinaryHeap<std::string> h;
  h.emplace(4, 'x');  // "xxxx"
  h.emplace(1, 'a');  // "a"
  h.emplace(2, 'b');  // "bb"
  ASSERT_EQ(h.size(), 3u);
  EXPECT_EQ(h.top(), "xxxx");
}

TEST(HeapTest, CustomComparatorYieldsMinHeap) {
  BinaryHeap<int, std::greater<int>> h;
  for (int x : {3, 7, 1, 5}) h.push(x);
  ASSERT_EQ(h.top(), 1);
  h.pop();
  EXPECT_EQ(h.top(), 3);
}

TEST(HeapTest, StructPayloadMaxByPriority) {
  BinaryHeap<Job> h;
  h.push({.priority = 1, .seq = 1});
  h.push({.priority = 9, .seq = 2});
  h.push({.priority = 5, .seq = 3});
  ASSERT_EQ(h.top().priority, 9);
}

TEST(HeapTest, ClearDropsContents) {
  BinaryHeap<int> h;
  for (int x : {3, 1, 2}) h.push(x);
  h.clear();
  EXPECT_TRUE(h.empty());
  EXPECT_THROW(h.top(), std::out_of_range);
}

TEST(HeapTest, HeapsortSortsAscending) {
  std::vector<int> a(100);
  std::iota(a.begin(), a.end(), 0);
  std::mt19937 rng(4242);
  std::shuffle(a.begin(), a.end(), rng);
  heapsort(a);
  for (std::size_t i = 1; i < a.size(); ++i) EXPECT_LE(a[i - 1], a[i]);

  std::vector<int> dup{5, 3, 5, 1, 3, 3, 2};
  heapsort(dup);
  EXPECT_EQ(dup, (std::vector<int>{1, 2, 3, 3, 3, 5, 5}));

  std::vector<int> sorted{1, 2, 3, 4};
  heapsort(sorted);
  EXPECT_EQ(sorted, (std::vector<int>{1, 2, 3, 4}));

  std::vector<int> rev{4, 3, 2, 1};
  heapsort(rev);
  EXPECT_EQ(rev, (std::vector<int>{1, 2, 3, 4}));

  std::vector<int> empty;
  heapsort(empty);
  EXPECT_TRUE(empty.empty());

  std::vector<int> single{9};
  heapsort(single);
  EXPECT_EQ(single, (std::vector<int>{9}));
}

TEST(HeapTest, HeapsortStrings) {
  std::vector<std::string> v{"beta", "alpha", "gamma", "delta"};
  heapsort(v);
  EXPECT_EQ(v, (std::vector<std::string>{"alpha", "beta", "delta", "gamma"}));
}

TEST(HeapTest, InterleavedPushPopReplaceKeepsHeap) {
  BinaryHeap<int> h;
  for (int x : {4, 9, 1, 7, 3, 8, 2, 6, 5, 0}) h.push(x);
  EXPECT_EQ(h.top(), 9);
  h.pop();
  EXPECT_EQ(h.top(), 8);
  h.replace(11);
  EXPECT_EQ(h.top(), 11);
  h.erase_at(1);
  EXPECT_TRUE(std::is_heap(h.array().begin(), h.array().end()));
  std::vector<int> got = drain(h);
  EXPECT_EQ(got.size(), 8u);
  for (std::size_t i = 1; i < got.size(); ++i) EXPECT_GE(got[i - 1], got[i]);
}