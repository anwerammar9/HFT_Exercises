#include "ohlcv_aggregator.h"

#include <gtest/gtest.h>

#include <chrono>

using namespace std::chrono_literals;

namespace {

void ExpectEmptyCandle(const OhlcvAggregator::Candle& c,
                       std::chrono::milliseconds open_time) {
  EXPECT_EQ(c.open_time, open_time);
  EXPECT_DOUBLE_EQ(c.open, 0.0);
  EXPECT_DOUBLE_EQ(c.high, 0.0);
  EXPECT_DOUBLE_EQ(c.low, 0.0);
  EXPECT_DOUBLE_EQ(c.close, 0.0);
  EXPECT_EQ(c.volume, 0u);
}

}  // namespace

TEST(OhlcvAggregatorTest, NonPositiveBucketThrows) {
  EXPECT_THROW(OhlcvAggregator(std::chrono::milliseconds(0)),
               std::invalid_argument);
  EXPECT_THROW(OhlcvAggregator(std::chrono::milliseconds(-1)),
               std::invalid_argument);
}

TEST(OhlcvAggregatorTest, FirstTradeOpensCandleAtBucketFloor) {
  OhlcvAggregator agg(std::chrono::milliseconds(1000));

  agg.add_trade(std::chrono::milliseconds(250), 10.0, 5);

  const auto& c = agg.current();
  EXPECT_EQ(c.open_time, std::chrono::milliseconds(0));  // floor(250/1000)=0
  EXPECT_DOUBLE_EQ(c.open, 10.0);
  EXPECT_DOUBLE_EQ(c.high, 10.0);
  EXPECT_DOUBLE_EQ(c.low, 10.0);
  EXPECT_DOUBLE_EQ(c.close, 10.0);
  EXPECT_EQ(c.volume, 5u);
  EXPECT_TRUE(agg.completed().empty());  // first trade: nothing finalized
}

TEST(OhlcvAggregatorTest, TradesWithinSameBucketFold) {
  OhlcvAggregator agg(std::chrono::milliseconds(100));

  agg.add_trade(std::chrono::milliseconds(10), 10.0, 5);
  agg.add_trade(std::chrono::milliseconds(90), 12.0, 2);
  agg.add_trade(std::chrono::milliseconds(49), 8.0, 1);  // same bucket 0

  const auto& c = agg.current();
  EXPECT_EQ(c.open_time, std::chrono::milliseconds(0));
  EXPECT_DOUBLE_EQ(c.open, 10.0);  // first trade of the bucket
  EXPECT_DOUBLE_EQ(c.high, 12.0);
  EXPECT_DOUBLE_EQ(c.low, 8.0);
  EXPECT_DOUBLE_EQ(c.close, 8.0);   // last trade wins
  EXPECT_EQ(c.volume, 8u);
}

TEST(OhlcvAggregatorTest, LaterBucketFinalizesPreviousAndEmitsGapEmpties) {
  OhlcvAggregator agg(std::chrono::milliseconds(100));

  agg.add_trade(std::chrono::milliseconds(10), 10.0, 1);   // bucket 0
  agg.add_trade(std::chrono::milliseconds(315), 20.0, 2);  // bucket 3

  const auto& completed = agg.completed();
  ASSERT_EQ(completed.size(), 3u);  // bucket 0 + empty buckets 1,2

  EXPECT_EQ(completed[0].open_time, std::chrono::milliseconds(0));
  EXPECT_DOUBLE_EQ(completed[0].open, 10.0);
  EXPECT_DOUBLE_EQ(completed[0].close, 10.0);
  EXPECT_EQ(completed[0].volume, 1u);
  ExpectEmptyCandle(completed[1], std::chrono::milliseconds(100));
  ExpectEmptyCandle(completed[2], std::chrono::milliseconds(200));

  const auto& cur = agg.current();
  EXPECT_EQ(cur.open_time, std::chrono::milliseconds(300));
  EXPECT_DOUBLE_EQ(cur.open, 20.0);
  EXPECT_DOUBLE_EQ(cur.high, 20.0);
  EXPECT_DOUBLE_EQ(cur.low, 20.0);
  EXPECT_DOUBLE_EQ(cur.close, 20.0);
  EXPECT_EQ(cur.volume, 2u);
}

TEST(OhlcvAggregatorTest, StaleOlderBucketTradeIsIgnored) {
  OhlcvAggregator agg(std::chrono::milliseconds(100));

  agg.add_trade(std::chrono::milliseconds(10), 10.0, 1);   // bucket 0
  agg.add_trade(std::chrono::milliseconds(315), 20.0, 2);  // bucket 3

  agg.add_trade(std::chrono::milliseconds(95), 999.0, 99);  // late: bucket 0

  EXPECT_EQ(agg.completed().size(), 3u);  // nothing new finalized
  const auto& c = agg.current();
  EXPECT_EQ(c.open_time, std::chrono::milliseconds(300));  // still bucket 3
  EXPECT_DOUBLE_EQ(c.open, 20.0);
  EXPECT_DOUBLE_EQ(c.close, 20.0);
  EXPECT_EQ(c.volume, 2u);
}

TEST(OhlcvAggregatorTest, FirstTradeInLaterBucketEmitsLeadingEmpties) {
  OhlcvAggregator agg(std::chrono::milliseconds(100));
  agg.add_trade(std::chrono::milliseconds(315), 20.0, 1);  // first = bucket 3

  const auto& completed = agg.completed();
  ASSERT_EQ(completed.size(), 3u);
  ExpectEmptyCandle(completed[0], std::chrono::milliseconds(0));
  ExpectEmptyCandle(completed[1], std::chrono::milliseconds(100));
  ExpectEmptyCandle(completed[2], std::chrono::milliseconds(200));

  const auto& cur = agg.current();
  EXPECT_EQ(cur.open_time, std::chrono::milliseconds(300));
  EXPECT_DOUBLE_EQ(cur.open, 20.0);
  EXPECT_EQ(cur.volume, 1u);
}

TEST(OhlcvAggregatorTest, RollFinalizesCurrentAndReturnsIt) {
  OhlcvAggregator agg(std::chrono::milliseconds(100));
  agg.add_trade(std::chrono::milliseconds(10), 10.0, 5);

  const auto rolled = agg.roll();
  EXPECT_EQ(rolled.open_time, std::chrono::milliseconds(0));
  EXPECT_DOUBLE_EQ(rolled.open, 10.0);
  EXPECT_DOUBLE_EQ(rolled.close, 10.0);
  EXPECT_EQ(rolled.volume, 5u);

  EXPECT_EQ(agg.completed().size(), 1u);
  const auto& cur = agg.current();
  EXPECT_EQ(cur.open_time, std::chrono::milliseconds(0));  // same bucket start
  EXPECT_EQ(cur.volume, 0u);  // fresh candle now
  EXPECT_DOUBLE_EQ(cur.open, 0.0);

  // The next trade opens a BRAND-NEW candle in the same bucket.
  agg.add_trade(std::chrono::milliseconds(90), 12.0, 1);
  const auto& again = agg.current();
  EXPECT_DOUBLE_EQ(again.open, 12.0);
  EXPECT_DOUBLE_EQ(again.open, again.close);
  EXPECT_EQ(again.volume, 1u);
}

TEST(OhlcvAggregatorTest, RollOnEmptyAggregatorIsNoOp) {
  OhlcvAggregator agg(std::chrono::milliseconds(100));
  const auto rolled = agg.roll();
  ExpectEmptyCandle(rolled, std::chrono::milliseconds(0));
  EXPECT_TRUE(agg.completed().empty());
  EXPECT_EQ(agg.current().open_time, std::chrono::milliseconds(0));
}