#include "token_bucket.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

TEST(TokenBucketTest, StartsFullAndDrains) {
  TokenBucket bucket(10.0, 5.0);
  for (int i = 0; i < 5; ++i) EXPECT_TRUE(bucket.try_acquire());  // stub: false -> red
  EXPECT_FALSE(bucket.try_acquire());  // drained
}

TEST(TokenBucketTest, ExactlyOneTokenPerIntervalRefills) {
  TokenBucket bucket(10.0, 1.0);  // 1 token per 100ms
  EXPECT_TRUE(bucket.try_acquire());  // start full (burst 1)
  EXPECT_FALSE(bucket.try_acquire());

  bucket.advance(50ms);
  EXPECT_FALSE(bucket.try_acquire());  // only 0.5 token grown
  bucket.advance(50ms);                // 100ms total -> exactly 1 token
  EXPECT_TRUE(bucket.try_acquire());
  EXPECT_FALSE(bucket.try_acquire());
}

TEST(TokenBucketTest, RefillCappedAtBurst) {
  TokenBucket bucket(100.0, 10.0);
  for (int i = 0; i < 10; ++i) EXPECT_TRUE(bucket.try_acquire());
  EXPECT_FALSE(bucket.try_acquire());

  bucket.advance(60s);  // far more than burst worth of time
  for (int i = 0; i < 10; ++i) EXPECT_TRUE(bucket.try_acquire());  // exactly burst
  EXPECT_FALSE(bucket.try_acquire());
}

TEST(TokenBucketTest, SustainedBudgetFollowsRate) {
  TokenBucket bucket(100.0, 20.0);
  for (int i = 0; i < 20; ++i) EXPECT_TRUE(bucket.try_acquire());  // drain start-full

  int accepted = 0;
  for (int ms = 0; ms < 1000; ms += 10) {
    bucket.advance(10ms);
    if (bucket.try_acquire()) ++accepted;
  }
  EXPECT_GE(accepted, 97);   // 100 tokens in 1s at 100/s, float tolerance
  EXPECT_LE(accepted, 100);
}

TEST(TokenBucketTest, ConcurrentCallersNeverExceedBudget) {
  // Drain a generous initial burst, then hand out exactly 200 tokens.
  TokenBucket bucket(500.0, 1000.0);
  for (int i = 0; i < 1000; ++i) EXPECT_TRUE(bucket.try_acquire());
  EXPECT_FALSE(bucket.try_acquire());
  bucket.advance(400ms);  // 0.4s * 500/s == 200 tokens

  std::atomic<int> accepted{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t) {
    threads.emplace_back([&] {
      for (int attempt = 0; attempt < 20000; ++attempt) {
        if (bucket.try_acquire()) accepted.fetch_add(1);
      }
    });
  }
  for (auto& th : threads) th.join();

  EXPECT_GT(accepted.load(), 0);      // stub: 0 -> red
  EXPECT_LE(accepted.load(), 200);    // budget never overspent
}

}  // namespace