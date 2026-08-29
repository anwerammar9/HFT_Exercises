#include <gtest/gtest.h>

#include "hash_map.h"

#include <cstdint>
#include <set>
#include <string>
#include <unordered_set>

namespace {

struct BadHash {  // every key lands in the same bucket: forces probing.
  std::size_t operator()(int) const noexcept { return 0; }
};

template <typename K, typename V>
using SmallMap = HashMap<K, V, BadHash>;

}  // namespace

TEST(HashMapTest, NewMapIsEmpty) {
  HashMap<int, int> m;
  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0u);
  EXPECT_EQ(m.begin(), m.end());
  EXPECT_EQ(m.bucket_count(), 0u);
  EXPECT_FALSE(m.contains(7));
}

TEST(HashMapTest, InsertNewAndExistingKey) {
  HashMap<int, int> m;
  auto [it, inserted] = m.insert({1, 100});
  ASSERT_TRUE(inserted);
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(it->second, 100);
  EXPECT_EQ(it->first, 1);

  auto [it2, inserted2] = m.insert({1, 200});
  EXPECT_FALSE(inserted2);
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(it2->second, 100);  // existing value untouched
}

TEST(HashMapTest, OperatorIndexInsertsAndOverwrites) {
  HashMap<std::string, int> m;
  m["aaa"] = 1;
  ASSERT_EQ(m.size(), 1u);
  EXPECT_EQ(m["aaa"], 1);

  m["aaa"] = 999;  // overwrite
  EXPECT_EQ(m["aaa"], 999);
  EXPECT_EQ(m.size(), 1u);

  m["bb"] = 2;  // distinct key -> distinct value in a real map
  ASSERT_EQ(m.size(), 2u);
  EXPECT_EQ(m["aaa"], 999);
  EXPECT_EQ(m["bb"], 2);
}

TEST(HashMapTest, AtThrowsOnMissingKey) {
  HashMap<std::string, int> m;
  m["k"] = 5;
  ASSERT_EQ(m.size(), 1u);
  EXPECT_EQ(m.at("k"), 5);
  EXPECT_THROW(m.at("nope"), std::out_of_range);
}

TEST(HashMapTest, FindAndContains) {
  HashMap<int, int> m;
  m[9] = 42;
  ASSERT_EQ(m.size(), 1u);

  EXPECT_TRUE(m.contains(9));
  EXPECT_FALSE(m.contains(8));
  EXPECT_EQ(m.find(9)->second, 42);
  EXPECT_EQ(m.find(8), m.end());
}

TEST(HashMapTest, EraseHitAndMiss) {
  HashMap<int, int> m;
  m[1] = 10;
  m[2] = 20;
  ASSERT_EQ(m.size(), 2u);

  EXPECT_TRUE(m.erase(1));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_FALSE(m.contains(1));
  EXPECT_EQ(m.find(1), m.end());
  EXPECT_TRUE(m.contains(2));

  EXPECT_FALSE(m.erase(1));  // second erase is a miss
  EXPECT_EQ(m.size(), 1u);
}

TEST(HashMapTest, GrowsAcrossRehashThreshold) {
  HashMap<int, int> m;
  for (int i = 0; i < 1000; ++i) m.insert({i, i * 3});
  ASSERT_EQ(m.size(), 1000u);

  // Rehash must have happened: capacity must exceed 1000 / 0.7.
  constexpr double kMinBuckets = 1000.0 / HashMap<int, int>::kMaxLoad + 1.0;
  EXPECT_GT(static_cast<double>(m.bucket_count()), kMinBuckets);
  for (int i = 0; i < 1000; ++i) {
    ASSERT_TRUE(m.contains(i)) << "key " << i;
    EXPECT_EQ(m.find(i)->second, i * 3);
  }

  m[1000] = -1;
  ASSERT_EQ(m.size(), 1001u);
}

TEST(HashMapTest, GrowsAboveReserveThreshold) {
  HashMap<int, int> m;
  m.reserve(2000);
  ASSERT_GE(m.bucket_count(), 2000u);
  for (int i = 0; i < 500; ++i) m[i] = i;
  ASSERT_EQ(m.size(), 500u);
  EXPECT_GE(m.bucket_count(), 2000u);  // reserve holds, no shrink on insert
}

TEST(HashMapTest, CollisionStormStaysCorrect) {
  SmallMap<int, int> m;  // BadHash: all keys share one bucket
  for (int i = 0; i < 500; ++i) m[i] = i + 1;
  ASSERT_EQ(m.size(), 500u);

  for (int i = 0; i < 500; ++i) {
    ASSERT_TRUE(m.contains(i)) << "key " << i;
    EXPECT_EQ(m[i], i + 1);
  }
}

TEST(HashMapTest, TombstonesReusedAfterErase) {
  HashMap<int, int> m;
  for (int i = 0; i < 200; ++i) m[i] = i;
  ASSERT_EQ(m.size(), 200u);

  for (int i = 0; i < 200; i += 2) EXPECT_TRUE(m.erase(i));
  ASSERT_EQ(m.size(), 100u);

  for (int i = 0; i < 200; i += 2) EXPECT_FALSE(m.find(i) != m.end());
  // New keys that may land on tombstoned slots must still work.
  for (int i = 1000; i < 1100; ++i) m[i] = -i;
  ASSERT_EQ(m.size(), 200u);

  for (int i = 0; i < 200; ++i) {
    if (i % 2 == 0) {
      EXPECT_FALSE(m.contains(i));
    } else {
      EXPECT_TRUE(m.contains(i));
      EXPECT_EQ(m.find(i)->second, i);
    }
  }
  for (int i = 1000; i < 1100; ++i) {
    EXPECT_TRUE(m.contains(i));
    EXPECT_EQ(m[i], -i);
  }
}

TEST(HashMapTest, IterationVisitsEveryElementOnce) {
  HashMap<int, int> m;
  for (int i = 0; i < 300; ++i) m[i] = i;
  ASSERT_EQ(m.size(), 300u);

  std::unordered_set<int> seen;
  for (auto it = m.begin(); it != m.end(); ++it) {
    EXPECT_TRUE(seen.insert(it->first).second) << "duplicate " << it->first;
    EXPECT_EQ(it->second, it->first);
  }
  EXPECT_EQ(seen.size(), 300u);
}

TEST(HashMapTest, ReserveGrowsBucketCount) {
  HashMap<int, int> m;
  m.reserve(512);
  ASSERT_GE(m.bucket_count(), 512u);
  m[1] = 1;
  ASSERT_EQ(m.size(), 1u);
  EXPECT_TRUE(m.contains(1));
}

TEST(HashMapTest, ClearKeepsCapacityButDropsContents) {
  HashMap<int, int> m;
  for (int i = 0; i < 100; ++i) m[i] = i;
  ASSERT_EQ(m.size(), 100u);
  std::size_t caps = m.bucket_count();

  m.clear();
  EXPECT_EQ(m.size(), 0u);
  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.bucket_count(), caps);  // storage retained
  EXPECT_EQ(m.begin(), m.end());
  EXPECT_FALSE(m.contains(0));
}