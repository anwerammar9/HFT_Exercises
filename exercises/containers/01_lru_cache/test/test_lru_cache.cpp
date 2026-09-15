#include "lru_cache.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>

TEST(LruCacheTest, MissingKeyReturnsNullopt) {
  LruCache<int, int> cache(4);
  EXPECT_FALSE(cache.get(42).has_value());
}

TEST(LruCacheTest, BasicGetPut) {
  LruCache<std::string, int> cache(3);
  cache.put("a", 1);
  cache.put("b", 2);

  EXPECT_EQ(cache.get("a"), std::optional<int>(1));
  EXPECT_EQ(cache.get("b"), std::optional<int>(2));
  EXPECT_FALSE(cache.get("c").has_value());
}

TEST(LruCacheTest, EvictsLeastRecentlyUsedNotOldestInserted) {
  // The classic LRU-distinct-from-FIFO test: touch "a" via get(), then insert
  // "d". Capacity 3 =>
  //     after puts a,b,c:  [ c, b, a ]   (a = MRU of the three? No: MRU=c, LRU=a)
  // order is MRU->LRU:     c b a
  // get("a") pulls a to front:           a c b  (LRU = b)
  // put("d") evicts b (the LRU) and d becomes MRU:  d a c
  // The key that gets evicted is the *untouched* one, not the oldest-inserted.
  LruCache<std::string, int> cache(3);
  cache.put("a", 1);
  cache.put("b", 2);
  cache.put("c", 3);

  EXPECT_EQ(cache.get("a"), std::optional<int>(1));  // touch a, recency up

  cache.put("d", 4);  // must evict b (untouched), not a or c

  EXPECT_EQ(cache.get("d"), std::optional<int>(4));
  EXPECT_EQ(cache.get("a"), std::optional<int>(1));  // a survived
  EXPECT_EQ(cache.get("c"), std::optional<int>(3));  // c survived
  EXPECT_FALSE(cache.get("b").has_value());          // b was evicted
}

TEST(LruCacheTest, UpdatingExistingKeyRefreshesRecency) {
  LruCache<std::string, int> cache(3);
  cache.put("a", 1);
  cache.put("b", 2);
  cache.put("c", 3);

  cache.put("a", 10);  // update a -> a becomes MRU, b is now LRU
  EXPECT_EQ(cache.get("a"), std::optional<int>(10));

  cache.put("d", 4);   // must evict b (oldest since a was refreshed)
  EXPECT_EQ(cache.get("d"), std::optional<int>(4));
  EXPECT_EQ(cache.get("a"), std::optional<int>(10));
  EXPECT_EQ(cache.get("c"), std::optional<int>(3));
  EXPECT_FALSE(cache.get("b").has_value());
}

TEST(LruCacheTest, RepeatedGetsDoNotEvict) {
  LruCache<int, int> cache(2);
  cache.put(1, 100);
  cache.put(2, 200);
  for (int i = 0; i < 100; ++i) EXPECT_EQ(cache.get(1), std::optional<int>(100));
  cache.put(3, 300);
  EXPECT_EQ(cache.get(1), std::optional<int>(100));  // 2 was evicted, not 1
  EXPECT_FALSE(cache.get(2).has_value());
  EXPECT_EQ(cache.get(3), std::optional<int>(300));
}

TEST(LruCacheTest, LargeWorkloadSmokeTest) {
  // Not an O(1) proof, but a smoke test: 100k mixed ops must finish in bounded
  // wall-clock so an accidental O(n) eviction gets caught.
  LruCache<int, int> cache(1'000);

  const auto start = std::chrono::steady_clock::now();
  unsigned int rng = 12345;
  int hits = 0;
  for (int i = 0; i < 100'000; ++i) {
    rng = (rng * 1103515245u + 12345u) & 0x7fffffffu;
    const int key = static_cast<int>(rng % 5000u);
    if (rng % 3 == 0) {
      cache.put(key, i);
    } else if (cache.get(key).has_value()) {
      ++hits;
    }
  }
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_GT(hits, 0) << "workload should have produced cache hits";
  EXPECT_LT(elapsed, std::chrono::seconds(5))
      << "100k ops took too long — smells like accidental O(n) eviction";
}