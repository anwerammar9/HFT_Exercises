#include "matching_engine.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

void expect_trade(const Trade& t, OrderId resting, OrderId aggressor, Price price,
                  Qty qty) {
  EXPECT_EQ(t.resting_id, resting);
  EXPECT_EQ(t.aggressor_id, aggressor);
  EXPECT_EQ(t.price, price);
  EXPECT_EQ(t.qty, qty);
}

}  // namespace

TEST(MatchingEngineTest, SingleCrossingOrderFillsAtRestingPrice) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Buy, 100, 5}).empty());  // rests

  const std::vector<Trade> trades = me.add_order({2, Side::Sell, 100, 5});
  ASSERT_EQ(trades.size(), 1u);
  // The RESTING (buy) price wins, not the aggressor's.
  expect_trade(trades[0], /*resting*/ 1, /*aggressor*/ 2, /*price*/ 100, /*qty*/ 5);

  // Book is empty afterwards: a new crossing sell finds nobody to match.
  EXPECT_TRUE(me.add_order({3, Side::Sell, 99, 5}).empty());
}

TEST(MatchingEngineTest, PartialFillLeavesRemainderRestingWithPriority) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Buy, 100, 10}).empty());  // rests 10

  // Aggressor smaller than resting: fully fills the aggressor, resting keeps
  // 6 and keeps its place at the head of the 100 level.
  const std::vector<Trade> t1 = me.add_order({2, Side::Sell, 100, 4});
  ASSERT_EQ(t1.size(), 1u);
  expect_trade(t1[0], 1, 2, 100, 4);

  // Next sell at the same price still matches the ORIGINAL resting order.
  const std::vector<Trade> t2 = me.add_order({3, Side::Sell, 100, 7});
  ASSERT_EQ(t2.size(), 1u);
  expect_trade(t2[0], 1, 3, 100, 6);  // order 3 rests 1 at 100

  // The reversed case: small resting order, big aggressor.
  EXPECT_TRUE(me.add_order({4, Side::Buy, 99, 2}).empty());   // rests 2
  const std::vector<Trade> t3 = me.add_order({5, Side::Sell, 99, 5});
  ASSERT_EQ(t3.size(), 1u);
  expect_trade(t3[0], 4, 5, 99, 2);   // order 5 now rests 3 at 99
}

TEST(MatchingEngineTest, PricePriorityFillsBetterPriceLevelFirst) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Sell, 101, 5}).empty());  // worse ask
  EXPECT_TRUE(me.add_order({2, Side::Sell, 100, 5}).empty());  // best ask

  const std::vector<Trade> trades = me.add_order({3, Side::Buy, 105, 5});
  ASSERT_EQ(trades.size(), 1u);
  expect_trade(trades[0], 2, 3, 100, 5);  // matched the BETTER (lower) ask
}

TEST(MatchingEngineTest, TimePriorityFifoWithinSamePriceLevel) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Buy, 100, 5}).empty());  // first bid @100
  EXPECT_TRUE(me.add_order({2, Side::Buy, 100, 5}).empty());  // second bid @100

  const std::vector<Trade> trades = me.add_order({3, Side::Sell, 100, 10});
  ASSERT_EQ(trades.size(), 2u);
  expect_trade(trades[0], 1, 3, 100, 5);  // FIFO: the earlier order fills first
  expect_trade(trades[1], 2, 3, 100, 5);
}

TEST(MatchingEngineTest, NonCrossingOrderRestsWithoutTrading) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Buy, 100, 5}).empty());   // best bid 100
  EXPECT_TRUE(me.add_order({2, Side::Buy, 99, 5}).empty());    // bid 99
  EXPECT_TRUE(me.add_order({3, Side::Sell, 101, 5}).empty());  // best ask 101
  EXPECT_TRUE(me.add_order({4, Side::Buy, 98, 5}).empty());    // below 99, rests
  EXPECT_TRUE(me.add_order({5, Side::Sell, 102, 5}).empty());  // above 101, rests
  EXPECT_TRUE(me.add_order({6, Side::Buy, 100, 5}).empty());   // 100 < best ask 101, rests

  // ...and a CROSSING order finally trades with the resting book (the 101 ask).
  const std::vector<Trade> trades = me.add_order({7, Side::Buy, 101, 5});
  ASSERT_EQ(trades.size(), 1u);
  expect_trade(trades[0], 3, 7, 101, 5);
}

TEST(MatchingEngineTest, CancelRemovesRestingOrder) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Buy, 100, 5}).empty());  // rests
  EXPECT_TRUE(me.cancel_order(1));
  EXPECT_FALSE(me.cancel_order(1));    // idempotent / already cancelled
  EXPECT_FALSE(me.cancel_order(999));  // unknown id, no throw

  EXPECT_TRUE(me.add_order({2, Side::Buy, 100, 5}).empty());  // rests
  const std::vector<Trade> trades = me.add_order({3, Side::Sell, 100, 5});
  ASSERT_EQ(trades.size(), 1u);
  expect_trade(trades[0], 2, 3, 100, 5);  // matched order 2, NOT the cancelled 1
}

TEST(MatchingEngineTest, MultiLevelSweepAcrossPriceLevels) {
  MatchingEngine me;

  EXPECT_TRUE(me.add_order({1, Side::Sell, 100, 2}).empty());
  EXPECT_TRUE(me.add_order({2, Side::Sell, 101, 3}).empty());
  EXPECT_TRUE(me.add_order({3, Side::Sell, 102, 4}).empty());

  // One big buy sweeps all three ask levels: 2 @100, then 3 @101, then 3 @102
  // (order 3 keeps a 1-lot remainder).
  const std::vector<Trade> trades = me.add_order({4, Side::Buy, 105, 8});
  ASSERT_EQ(trades.size(), 3u);
  expect_trade(trades[0], 1, 4, 100, 2);
  expect_trade(trades[1], 2, 4, 101, 3);
  expect_trade(trades[2], 3, 4, 102, 3);

  // Order 3's 1-lot remainder is still resting and still fills first:
  EXPECT_TRUE(me.add_order({5, Side::Sell, 103, 5}).empty());  // non-crossing
  const std::vector<Trade> t2 = me.add_order({6, Side::Buy, 105, 1});
  ASSERT_EQ(t2.size(), 1u);
  expect_trade(t2[0], 3, 6, 102, 1);
}