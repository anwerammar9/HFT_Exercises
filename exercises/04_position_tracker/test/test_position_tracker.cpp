#include "position_tracker.h"

#include <gtest/gtest.h>

namespace {

TEST(PositionTrackerTest, BuysAverageIntoCost) {
  PositionTracker pt;
  pt.on_fill(Side::Buy, 100, 10.0);
  pt.on_fill(Side::Buy, 100, 12.0);
  EXPECT_EQ(pt.position(), 200);
  EXPECT_NEAR(pt.avg_cost(), 11.0, 1e-9);
  EXPECT_NEAR(pt.realized_pnl(), 0.0, 1e-9);
}

TEST(PositionTrackerTest, PartialCloseRealizesPnl) {
  PositionTracker pt;
  pt.on_fill(Side::Buy, 100, 10.0);
  pt.on_fill(Side::Buy, 100, 12.0);  // avg cost 11
  pt.on_fill(Side::Sell, 100, 15.0);

  EXPECT_EQ(pt.position(), 100);
  EXPECT_NEAR(pt.avg_cost(), 11.0, 1e-9);  // basis unchanged by the partial close
  EXPECT_NEAR(pt.realized_pnl(), 400.0, 1e-9);  // (15 - 11) * 100
}

TEST(PositionTrackerTest, FullCloseThenFlipSplitsCloseAndOpen) {
  PositionTracker pt;
  pt.on_fill(Side::Buy, 100, 10.0);
  pt.on_fill(Side::Sell, 150, 15.0);  // closes 100-long, opens 50-short

  EXPECT_EQ(pt.position(), -50);
  EXPECT_NEAR(pt.realized_pnl(), 500.0, 1e-9);  // (15 - 10) * 100
  EXPECT_NEAR(pt.avg_cost(), 15.0, 1e-9);       // new short basis
}

TEST(PositionTrackerTest, UnrealizedPnlSignedBothSides) {
  PositionTracker long_pt;
  long_pt.on_fill(Side::Buy, 100, 10.0);
  EXPECT_NEAR(long_pt.unrealized_pnl(12.0), 200.0, 1e-9);  // (12 - 10) * 100

  PositionTracker short_pt;
  short_pt.on_fill(Side::Sell, 50, 20.0);
  EXPECT_NEAR(short_pt.unrealized_pnl(16.0), 200.0, 1e-9);  // (16 - 20) * -50
}

TEST(PositionTrackerTest, FlatContract) {
  PositionTracker pt;
  pt.on_fill(Side::Buy, 100, 10.0);
  pt.on_fill(Side::Sell, 100, 14.0);

  EXPECT_EQ(pt.position(), 0);
  EXPECT_NEAR(pt.unrealized_pnl(100.0), 0.0, 1e-9);  // flat -> 0
  EXPECT_NEAR(pt.avg_cost(), 0.0, 1e-9);             // flat -> 0 (contract)
  EXPECT_NEAR(pt.realized_pnl(), 400.0, 1e-9);       // (14 - 10) * 100
}

}  // namespace