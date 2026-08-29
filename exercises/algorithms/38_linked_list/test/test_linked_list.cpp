#include <gtest/gtest.h>

#include "linked_list.h"

#include <string>
#include <vector>

namespace {

template <typename T>
std::vector<T> collect(const LinkedList<T>& l) {
  std::vector<T> out;
  for (const T& v : l) out.push_back(v);
  return out;
}

}  // namespace

TEST(LinkedListTest, NewListIsEmpty) {
  LinkedList<int> l;
  EXPECT_TRUE(l.empty());
  EXPECT_EQ(l.size(), 0u);
  EXPECT_THROW(l.front(), std::out_of_range);
  EXPECT_THROW(l.back(), std::out_of_range);
  EXPECT_TRUE(l.begin() == l.end());
}

TEST(LinkedListTest, PushBackPreservesOrder) {
  LinkedList<int> l;
  for (int i = 1; i <= 5; ++i) l.push_back(i);
  ASSERT_EQ(l.size(), 5u);
  EXPECT_EQ(l.front(), 1);
  EXPECT_EQ(l.back(), 5);
  EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(LinkedListTest, PushFrontPrepends) {
  LinkedList<int> l;
  l.push_back(3);
  l.push_front(2);
  l.push_front(1);
  EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(l.front(), 1);
}

TEST(LinkedListTest, PushBackMoveTakesOwnership) {
  LinkedList<std::string> l;
  std::string s = "order-42";
  l.push_back(std::move(s));
  ASSERT_EQ(l.back(), "order-42");
  EXPECT_EQ(l.size(), 1u);
}

TEST(LinkedListTest, PopUpdatesBothEnds) {
  LinkedList<int> l;
  for (int i = 1; i <= 4; ++i) l.push_back(i);
  l.pop_front();
  EXPECT_EQ(l.front(), 2);
  EXPECT_EQ(l.size(), 3u);
  l.pop_back();
  EXPECT_EQ(l.back(), 3);
  EXPECT_EQ(l.size(), 2u);
  l.pop_front();
  l.pop_front();
  EXPECT_TRUE(l.empty());
}

TEST(LinkedListTest, BidirectionalIteration) {
  LinkedList<int> l;
  for (int i = 1; i <= 4; ++i) l.push_back(i);
  auto it = l.end();
  --it;
  EXPECT_EQ(*it, 4);
  --it;
  EXPECT_EQ(*it, 3);
  ++it;
  EXPECT_EQ(*it, 4);
}

TEST(LinkedListTest, EraseMiddleIsO1AndReturnsNext) {
  LinkedList<int> l;
  for (int i = 1; i <= 5; ++i) l.push_back(i);
  auto it = l.begin();
  ++it;
  ++it;  // -> 3
  auto next = l.erase(it);
  ASSERT_EQ(l.size(), 4u);
  EXPECT_EQ(*next, 4);
  EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 4, 5}));
}

TEST(LinkedListTest, EraseFirstAndLastNodes) {
  LinkedList<int> l;
  for (int i = 1; i <= 3; ++i) l.push_back(i);
  l.erase(l.begin());  // removes 1
  EXPECT_EQ(collect(l), (std::vector<int>{2, 3}));
  l.erase(--l.end());  // removes 3
  EXPECT_EQ(collect(l), (std::vector<int>{2}));
  EXPECT_EQ(l.front(), 2);
}

TEST(LinkedListTest, EraseLoopIdiomDrainsList) {
  LinkedList<int> l;
  for (int i = 0; i < 5; ++i) l.push_back(i);
  ASSERT_EQ(l.size(), 5u);  // guards a no-op push_back that would pass trivially
  for (auto it = l.begin(); it != l.end();) it = l.erase(it);
  EXPECT_TRUE(l.empty());
  EXPECT_EQ(l.size(), 0u);
}

TEST(LinkedListTest, EraseLastNodeReturnsEnd) {
  LinkedList<int> l;
  l.push_back(1);
  ASSERT_EQ(l.back(), 1);  // guards a no-op push_back that would pass trivially
  auto after = l.erase(l.begin());
  EXPECT_TRUE(after == l.end());
  EXPECT_TRUE(l.empty());
}

TEST(LinkedListTest, ReverseFlipsOrder) {
  LinkedList<int> l;
  for (int i = 1; i <= 4; ++i) l.push_back(i);
  l.reverse();
  EXPECT_EQ(collect(l), (std::vector<int>{4, 3, 2, 1}));
  EXPECT_EQ(l.front(), 4);
  EXPECT_EQ(l.back(), 1);
}

TEST(LinkedListTest, ReverseSingleAndEmptyAreNoOps) {
  LinkedList<int> empty;
  empty.reverse();
  EXPECT_TRUE(empty.empty());

  LinkedList<int> one;
  one.push_back(7);
  one.reverse();
  EXPECT_EQ(collect(one), (std::vector<int>{7}));
}

TEST(LinkedListTest, ReverseAfterOpsKeepsInvariant) {
  LinkedList<int> l;
  for (int i = 1; i <= 6; ++i) l.push_back(i);
  l.pop_front();
  l.pop_back();
  l.push_front(0);
  l.reverse();
  EXPECT_EQ(collect(l), (std::vector<int>{5, 4, 3, 2, 0}));
}

TEST(LinkedListTest, CopyIsDeep) {
  LinkedList<int> l;
  for (int i = 1; i <= 3; ++i) l.push_back(i);
  LinkedList<int> copy(l);
  copy.push_back(99);
  copy.pop_front();
  EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(collect(copy), (std::vector<int>{2, 3, 99}));
  EXPECT_EQ(l.size(), 3u);
}

TEST(LinkedListTest, MoveLeavesSourceEmpty) {
  LinkedList<std::string> l;
  l.push_back("a");
  l.push_back("b");
  LinkedList<std::string> moved(std::move(l));
  EXPECT_EQ(collect(moved), (std::vector<std::string>{"a", "b"}));
  EXPECT_TRUE(l.empty());
}

TEST(LinkedListTest, CopyAssignReplacesContent) {
  LinkedList<int> a, b;
  for (int i = 1; i <= 3; ++i) a.push_back(i);
  b.push_back(42);
  b = a;
  EXPECT_EQ(collect(b), (std::vector<int>{1, 2, 3}));
  a.pop_back();
  EXPECT_EQ(collect(b), (std::vector<int>{1, 2, 3}));  // still deep
}

TEST(LinkedListTest, ClearFreesAndEmpties) {
  LinkedList<int> l;
  for (int i = 0; i < 8; ++i) l.push_back(i);
  l.clear();
  EXPECT_TRUE(l.empty());
  l.push_back(5);  // usable again after clear
  EXPECT_EQ(collect(l), (std::vector<int>{5}));
}

TEST(LinkedListTest, StressEraseEveryOtherNode) {
  constexpr int kCount = 10'000;
  LinkedList<int> l;
  for (int i = 0; i < kCount; ++i) l.push_back(i);
  auto it = l.begin();
  bool skip = false;
  while (it != l.end()) {
    if (!skip) {
      it = l.erase(it);
    } else {
      ++it;
    }
    skip = !skip;
  }
  EXPECT_EQ(l.size(), kCount / 2);
  EXPECT_EQ(l.front(), 1);
  EXPECT_EQ(l.back(), kCount - 1);
}

TEST(LinkedListTest, StringPayloadRoundTrip) {
  LinkedList<std::string> l;
  l.push_back("iex");
  l.push_back("cme");
  l.push_back("binance");
  EXPECT_EQ(l.front(), "iex");
  EXPECT_EQ(l.back(), "binance");
  auto it = l.begin();
  EXPECT_EQ(*it, "iex");
  ++it;
  EXPECT_EQ(it->size(), 3u);
}