#include <gtest/gtest.h>

#include "order_book.h"

#include <cstdint>

namespace {

// Fill a book with a canonical two-sided shape so individual tests stay short.
void build_two_sided(OrderBook& ob) {
  ob.add_order(1, Side::kBuy, 99.5, 5);
  ob.add_order(2, Side::kBuy, 100.0, 10);  // best bid
  ob.add_order(3, Side::kSell, 100.5, 7);
  ob.add_order(4, Side::kSell, 101.0, 3);  // best ask
  ob.add_order(5, Side::kBuy, 99.5, 2);
}

}  // namespace

TEST(OrderBookTest, EmptyBookHasNoTopOfBook) {
  OrderBook ob;
  EXPECT_FALSE(ob.best_bid().has_value());
  EXPECT_FALSE(ob.best_ask().has_value());
  EXPECT_EQ(ob.order_count(), 0u);
}

TEST(OrderBookTest, AddOrdersAndReadTopOfBook) {
  OrderBook ob;
  build_two_sided(ob);

  auto bid = ob.best_bid();
  ASSERT_TRUE(bid.has_value());
  EXPECT_EQ(bid->side, Side::kBuy);
  EXPECT_EQ(bid->price, 100.0);
  EXPECT_EQ(bid->qty, 10u);

  auto ask = ob.best_ask();
  ASSERT_TRUE(ask.has_value());
  EXPECT_EQ(ask->side, Side::kSell);
  EXPECT_EQ(ask->price, 100.5);
  EXPECT_EQ(ask->qty, 7u);

  ASSERT_TRUE(ob.qty_at(Side::kBuy, 99.5).has_value());
  EXPECT_EQ(ob.qty_at(Side::kBuy, 99.5).value(), 7u);  // orders 1 + 5
  EXPECT_EQ(ob.order_count(), 5u);
}

TEST(OrderBookTest, RejectsDuplicateOrderId) {
  OrderBook ob;
  ASSERT_TRUE(ob.add_order(1, Side::kBuy, 100.0, 5));
  EXPECT_FALSE(ob.add_order(1, Side::kSell, 101.0, 5));
  EXPECT_EQ(ob.order_count(), 1u);
}

TEST(OrderBookTest, MarketOrderFillsAtRestingPrice) {
  OrderBook ob;
  ASSERT_TRUE(ob.add_order(1, Side::kBuy, 100.0, 10));

  FillResult r = ob.market_order(Side::kSell, 6);
  ASSERT_EQ(r.trades.size(), 1u);
  EXPECT_EQ(r.trades[0].buy_order, 1u);
  EXPECT_EQ(r.trades[0].price, 100.0);  // resting price, not aggressor's
  EXPECT_EQ(r.trades[0].qty, 6u);
  EXPECT_EQ(r.remaining_qty, 0u);

  // The untouched 4 remain resting at the same price.
  ASSERT_TRUE(ob.qty_at(Side::kBuy, 100.0).has_value());
  EXPECT_EQ(ob.qty_at(Side::kBuy, 100.0).value(), 4u);
}

TEST(OrderBookTest, MarketOrderSweepsLevelsBestFirst) {
  OrderBook ob;
  ASSERT_TRUE(ob.add_order(1, Side::kBuy, 100.0, 5));
  ASSERT_TRUE(ob.add_order(2, Side::kBuy, 99.0, 7));
  ASSERT_TRUE(ob.add_order(3, Side::kBuy, 98.0, 2));

  FillResult r = ob.market_order(Side::kSell, 10);
  ASSERT_EQ(r.trades.size(), 2u);
  EXPECT_EQ(r.trades[0].buy_order, 1u);  // best level first
  EXPECT_EQ(r.trades[0].price, 100.0);
  EXPECT_EQ(r.trades[0].qty, 5u);
  EXPECT_EQ(r.trades[1].buy_order, 2u);
  EXPECT_EQ(r.trades[1].price, 99.0);
  EXPECT_EQ(r.trades[1].qty, 5u);
  EXPECT_EQ(r.remaining_qty, 0u);

  ASSERT_TRUE(ob.qty_at(Side::kBuy, 99.0).has_value());
  EXPECT_EQ(ob.qty_at(Side::kBuy, 99.0).value(), 2u);
  ASSERT_TRUE(ob.qty_at(Side::kBuy, 98.0).has_value());
  EXPECT_EQ(ob.qty_at(Side::kBuy, 98.0).value(), 2u);
}

TEST(OrderBookTest, MarketOrderLeftoverIsReported) {
  OrderBook ob;
  ASSERT_TRUE(ob.add_order(1, Side::kBuy, 100.0, 5));

  FillResult r = ob.market_order(Side::kSell, 20);
  ASSERT_EQ(r.trades.size(), 1u);
  EXPECT_EQ(r.trades[0].qty, 5u);
  EXPECT_EQ(r.remaining_qty, 15u);
}

TEST(OrderBookTest, CancelPreservesFifoOfRemaining) {
  OrderBook ob;
  ASSERT_TRUE(ob.add_order(1, Side::kBuy, 100.0, 10));
  ASSERT_TRUE(ob.add_order(2, Side::kBuy, 100.0, 10));

  // Cancel the earlier order; the later one must now be first in line.
  ASSERT_TRUE(ob.cancel_order(1));
  EXPECT_FALSE(ob.cancel_order(1));  // already gone
  EXPECT_EQ(ob.order_count(), 1u);

  FillResult r = ob.market_order(Side::kSell, 10);
  ASSERT_EQ(r.trades.size(), 1u);
  EXPECT_EQ(r.trades[0].buy_order, 2u);
  EXPECT_EQ(r.remaining_qty, 0u);
  EXPECT_TRUE(ob.best_bid().has_value() == false);
}

TEST(OrderBookTest, ModifyKeepsTimePriority) {
  OrderBook ob;
  ASSERT_TRUE(ob.add_order(1, Side::kBuy, 100.0, 10));
  ASSERT_TRUE(ob.add_order(2, Side::kBuy, 100.0, 10));

  // Order 1 was first; raising its qty must not demote it behind order 2.
  ASSERT_TRUE(ob.modify_order(1, 25));
  EXPECT_FALSE(ob.modify_order(99, 1));  // unknown id

  FillResult r = ob.market_order(Side::kSell, 10);
  ASSERT_EQ(r.trades.size(), 1u);
  EXPECT_EQ(r.trades[0].buy_order, 1u);
  EXPECT_EQ(r.trades[0].qty, 10u);
}

TEST(OrderBookTest, MarketOrderAgainstEmptyBookLeavesRemainder) {
  OrderBook ob;
  FillResult r = ob.market_order(Side::kBuy, 5);
  EXPECT_TRUE(r.trades.empty());
  EXPECT_EQ(r.remaining_qty, 5u);
}

TEST(OrderBookTest, ClearResetsEverything) {
  OrderBook ob;
  build_two_sided(ob);
  ASSERT_EQ(ob.order_count(), 5u);

  ob.clear();
  EXPECT_EQ(ob.order_count(), 0u);
  EXPECT_FALSE(ob.best_bid().has_value());
  EXPECT_FALSE(ob.best_ask().has_value());
}