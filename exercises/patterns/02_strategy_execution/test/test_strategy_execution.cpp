#include "strategy_execution.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::vector<ChildOrder> run_once(ExecutionStrategy& strategy,
                                 const ExecutionContext& ctx) {
  std::vector<ChildOrder> out;
  strategy.run(ctx, [&out](const ChildOrder& c) { out.push_back(c); });
  return out;
}

std::int64_t sum_qty(const std::vector<ChildOrder>& orders) {
  std::int64_t total = 0;
  for (const auto& o : orders) total += o.qty;
  return total;
}

TEST(StrategyExecutionTest, TwapSplitsExactlyLastGetsRemainder) {
  TwapExecutionStrategy twap;
  ExecutionContext ctx;
  ctx.parent_id = 5;
  ctx.side = Side::kBuy;
  ctx.total_qty = 100;
  ctx.slices = 3;

  auto orders = run_once(twap, ctx);
  ASSERT_EQ(orders.size(), 3u);  // stub: none emitted -> red
  EXPECT_EQ(orders[0].qty, 33);
  EXPECT_EQ(orders[1].qty, 33);
  EXPECT_EQ(orders[2].qty, 34);  // the last slice absorbs the remainder
  EXPECT_EQ(sum_qty(orders), 100);
  for (std::size_t i = 0; i < orders.size(); ++i) {
    EXPECT_EQ(orders[i].parent_id, 5);
    EXPECT_EQ(orders[i].side, Side::kBuy);
    EXPECT_EQ(orders[i].seq, static_cast<std::int64_t>(i + 1));
    EXPECT_EQ(orders[i].price, 0);  // no limit -> market prints
  }
}

TEST(StrategyExecutionTest, TwapCarriesLimitPrice) {
  TwapExecutionStrategy twap;
  ExecutionContext ctx;
  ctx.total_qty = 10;
  ctx.slices = 2;
  ctx.limit = 1'234'000;

  auto orders = run_once(twap, ctx);
  ASSERT_EQ(orders.size(), 2u);  // stub: none emitted -> red
  for (const auto& o : orders) EXPECT_EQ(o.price, 1'234'000);
}

TEST(StrategyExecutionTest, TwapZeroSlicesEmitsNothing) {
  TwapExecutionStrategy twap;
  ExecutionContext ctx;
  ctx.total_qty = 100;
  ctx.slices = 0;
  EXPECT_TRUE(run_once(twap, ctx).empty());
}

TEST(StrategyExecutionTest, VwapUsesLargestRemainder) {
  VwapExecutionStrategy vwap;
  ExecutionContext ctx;
  ctx.parent_id = 7;
  ctx.side = Side::kSell;
  ctx.total_qty = 11;
  ctx.weights = {0.125, 0.375, 0.5};

  auto orders = run_once(vwap, ctx);
  ASSERT_EQ(orders.size(), 3u);  // stub: none emitted -> red
  EXPECT_EQ(orders[0].qty, 1);
  EXPECT_EQ(orders[1].qty, 4);
  EXPECT_EQ(orders[2].qty, 6);  // 0.5 remainder wins the single leftover unit
  EXPECT_EQ(sum_qty(orders), 11);
}

TEST(StrategyExecutionTest, VwapSkipsZeroWeightBuckets) {
  VwapExecutionStrategy vwap;
  ExecutionContext ctx;
  ctx.total_qty = 100;
  ctx.weights = {0, 1, 0};

  auto orders = run_once(vwap, ctx);
  ASSERT_EQ(orders.size(), 1u);  // stub: none emitted -> red
  EXPECT_EQ(orders[0].qty, 100);
  EXPECT_EQ(orders[0].seq, 1);
}

TEST(StrategyExecutionTest, VwapEmptyOrZeroWeightsEmitsNothing) {
  VwapExecutionStrategy vwap;
  ExecutionContext ctx;
  ctx.total_qty = 100;
  EXPECT_TRUE(run_once(vwap, ctx).empty());
  ctx.weights = {0.0, 0.0};
  EXPECT_TRUE(run_once(vwap, ctx).empty());
}

TEST(StrategyExecutionTest, SniperSendsSingleFullSizeChild) {
  SniperExecutionStrategy sniper;
  ExecutionContext ctx;
  ctx.parent_id = 9;
  ctx.side = Side::kBuy;
  ctx.total_qty = 77;
  ctx.limit = 5'000'000;
  ctx.post_only = true;

  auto orders = run_once(sniper, ctx);
  ASSERT_EQ(orders.size(), 1u);  // stub: none emitted -> red
  EXPECT_EQ(orders[0].qty, 77);
  EXPECT_EQ(orders[0].price, 5'000'000);
  EXPECT_EQ(orders[0].seq, 1);
  EXPECT_TRUE(orders[0].post_only);
}

TEST(StrategyExecutionTest, SniperMarketsWhenNoLimit) {
  SniperExecutionStrategy sniper;
  ExecutionContext ctx;
  ctx.total_qty = 77;
  auto orders = run_once(sniper, ctx);
  ASSERT_EQ(orders.size(), 1u);  // stub: none emitted -> red
  EXPECT_EQ(orders[0].price, 0);
}

TEST(StrategyExecutionTest, RegistrySelectsByCaseInsensitiveName) {
  auto t = make_strategy("twap");
  auto v = make_strategy("VWAP");
  auto s = make_strategy("Sniper");
  ASSERT_NE(t, nullptr);
  ASSERT_NE(v, nullptr);
  ASSERT_NE(s, nullptr);
  EXPECT_STREQ(t->name(), "twap");
  EXPECT_STREQ(v->name(), "vwap");  // stub: always "twap" -> red
  EXPECT_STREQ(s->name(), "sniper");
}

TEST(StrategyExecutionTest, UnknownStrategyThrows) {
  EXPECT_THROW(make_strategy("iceberg"), std::invalid_argument);
  EXPECT_THROW(make_strategy(""), std::invalid_argument);
}

TEST(StrategyExecutionTest, EngineDelegatesAndSwaps) {
  ExecutionEngine engine(make_strategy("twap"));
  ExecutionContext ctx;
  ctx.total_qty = 100;
  ctx.slices = 2;

  std::vector<ChildOrder> out;
  engine.execute(ctx, [&out](const ChildOrder& c) { out.push_back(c); });
  EXPECT_EQ(out.size(), 2u);  // stub: none emitted -> red
  EXPECT_STREQ(engine.strategy_name(), "twap");

  engine.set_strategy(make_strategy("vwap"));
  ctx.weights = {1.0};
  out.clear();
  engine.execute(ctx, [&out](const ChildOrder& c) { out.push_back(c); });
  ASSERT_EQ(out.size(), 1u);  // stub: none emitted -> red
  EXPECT_EQ(out[0].qty, 100);
  EXPECT_STREQ(engine.strategy_name(), "vwap");  // stub: "twap" -> red
}

}  // namespace