#include <gtest/gtest.h>

#include "tick_statistics.h"

#include <cmath>
#include <cstdint>

TEST(TickStatisticsTest, EmptyStateIsDefined) {
  TickStatistics ts;
  EXPECT_EQ(ts.count(), 0u);
  EXPECT_EQ(ts.last(), 0.0);
  EXPECT_EQ(ts.min(), 0.0);
  EXPECT_EQ(ts.max(), 0.0);
  EXPECT_EQ(ts.mean(), 0.0);
  EXPECT_EQ(ts.variance(), 0.0);
  EXPECT_EQ(ts.vwap(), 0.0);
  EXPECT_EQ(ts.ewma(0.5), 0.0);
}

TEST(TickStatisticsTest, SingleTick) {
  TickStatistics ts;
  ts.add_tick(100.5, 10);
  EXPECT_EQ(ts.count(), 1u);
  EXPECT_DOUBLE_EQ(ts.last(), 100.5);
  EXPECT_DOUBLE_EQ(ts.min(), 100.5);
  EXPECT_DOUBLE_EQ(ts.max(), 100.5);
  EXPECT_DOUBLE_EQ(ts.mean(), 100.5);
  EXPECT_DOUBLE_EQ(ts.variance(), 0.0);
  EXPECT_DOUBLE_EQ(ts.vwap(), 100.5);
  EXPECT_DOUBLE_EQ(ts.ewma(0.5), 100.5);
}

TEST(TickStatisticsTest, ExactMeanVarianceMinMax) {
  TickStatistics ts;
  // 2, 4, 4, 4, 5, 5, 7, 9 -> mean 5, population variance 4.
  for (double p : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) ts.add_tick(p, 1);
  EXPECT_EQ(ts.count(), 8u);
  EXPECT_DOUBLE_EQ(ts.last(), 9.0);
  EXPECT_DOUBLE_EQ(ts.min(), 2.0);
  EXPECT_DOUBLE_EQ(ts.max(), 9.0);
  EXPECT_DOUBLE_EQ(ts.mean(), 5.0);
  EXPECT_DOUBLE_EQ(ts.variance(), 4.0);
}

TEST(TickStatisticsTest, MinMaxTrackTheExtremes) {
  TickStatistics ts;
  ts.add_tick(10, 1);
  ts.add_tick(20, 1);  // max widens
  ts.add_tick(5, 1);   // min tightens
  EXPECT_DOUBLE_EQ(ts.min(), 5.0);
  EXPECT_DOUBLE_EQ(ts.max(), 20.0);
  EXPECT_DOUBLE_EQ(ts.mean(), (10 + 20 + 5) / 3.0);
}

TEST(TickStatisticsTest, VwapWeightsByQuantity) {
  TickStatistics ts;
  ts.add_tick(10, 3);
  ts.add_tick(20, 1);
  EXPECT_DOUBLE_EQ(ts.vwap(), (10 * 3 + 20 * 1) / 4.0);
  EXPECT_DOUBLE_EQ(ts.mean(), 15.0);  // arithmetic mean of prices, not qty
}

TEST(TickStatisticsTest, EwmaFollowsTheFormula) {
  TickStatistics ts;
  // alpha = 0.5 over 2, 4, 8:
  //   ema = 2, then 0.5*4+0.5*2 = 3, then 0.5*8+0.5*3 = 5.5.
  ts.add_tick(2, 1);
  ts.add_tick(4, 1);
  ts.add_tick(8, 1);
  EXPECT_DOUBLE_EQ(ts.ewma(0.5), 5.5);
  EXPECT_DOUBLE_EQ(ts.ewma(1.0), 8.0);  // pure alpha: just the last
  EXPECT_DOUBLE_EQ(ts.ewma(0.0), 2.0);  // never moves: the seed
}

TEST(TickStatisticsTest, ManyTicksStayExact) {
  TickStatistics ts;
  constexpr int kTicks = 1000;
  for (int i = 0; i < kTicks; ++i) {
    ts.add_tick(static_cast<double>(i % 100), 2);  // uniform 0..99, qty 2
  }
  EXPECT_EQ(ts.count(), static_cast<std::uint64_t>(kTicks));
  EXPECT_DOUBLE_EQ(ts.min(), 0.0);
  EXPECT_DOUBLE_EQ(ts.max(), 99.0);
  EXPECT_DOUBLE_EQ(ts.mean(), 49.5);
  // Population variance of the integers 0..99 is (n^2-1)/12 = 833.25.
  EXPECT_NEAR(ts.variance(), 833.25, 1e-6);
  EXPECT_DOUBLE_EQ(ts.vwap(), 49.5);
}