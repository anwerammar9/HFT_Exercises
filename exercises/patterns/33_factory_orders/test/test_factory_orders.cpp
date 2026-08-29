#include "factory_orders.h"

#include <memory>

#include <gtest/gtest.h>

namespace {

constexpr PriceMicros kPrice12_345 = 12'345'000;  // 12.345
constexpr PriceMicros kPrice12_344 = 12'344'444;  // 12.344444

TEST(FactoryOrdersTest, IexRoundsHalfUpToCent) {
  IexOrderFactory f;
  auto o = f.create_limit(7, Side::kBuy, kPrice12_345, 100, TimeInForce::kDay,
                          true);
  ASSERT_NE(o, nullptr);  // stub: nullptr -> red
  EXPECT_EQ(o->price, 12'350'000);  // 12.35 (rounded up half to the cent)
  EXPECT_EQ(o->kind, OrderKind::kLimit);
  EXPECT_EQ(o->venue, Venue::kIex);
  EXPECT_EQ(o->side, Side::kBuy);
  EXPECT_EQ(o->id, 7);
  EXPECT_EQ(o->qty, 100);
  EXPECT_EQ(o->tif, TimeInForce::kDay);
  EXPECT_TRUE(o->post_only);
}

TEST(FactoryOrdersTest, CmeUsesMinimumTickNotCent) {
  CmeOrderFactory f;
  auto o = f.create_limit(7, Side::kBuy, kPrice12_345, 100, TimeInForce::kDay,
                          false);
  ASSERT_NE(o, nullptr);  // stub: nullptr -> red
  EXPECT_EQ(o->price, 12'345'000);  // 12.345 is already on the 0.0025 grid
  EXPECT_EQ(o->venue, Venue::kCme);
}

TEST(FactoryOrdersTest, IexAndCmeQuantizeSameInputDifferently) {
  const PriceMicros input = kPrice12_344;
  auto a = IexOrderFactory().create_limit(1, Side::kBuy, input, 10,
                                          TimeInForce::kDay, false);
  auto b = CmeOrderFactory().create_limit(1, Side::kBuy, input, 10,
                                          TimeInForce::kDay, false);
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(a->price, 12'340'000);  // 12.34 on the cent grid
  EXPECT_EQ(b->price, 12'345'000);  // 12.344444 -> up to 12.345 on 0.0025
  EXPECT_NE(a->price, b->price);    // the same request lands on a DIFFERENT grid
}

TEST(FactoryOrdersTest, RejectsZeroAndNegativeQty) {
  IexOrderFactory f;
  EXPECT_EQ(f.create_limit(1, Side::kBuy, 1000, 0, TimeInForce::kDay, false),
            nullptr);
  EXPECT_EQ(f.create_limit(1, Side::kBuy, 1000, -5, TimeInForce::kDay, false),
            nullptr);
  EXPECT_EQ(f.create_market(1, Side::kBuy, 0), nullptr);
  EXPECT_EQ(f.create_stop(1, Side::kBuy, 1000, -1), nullptr);
}

TEST(FactoryOrdersTest, RejectsNonPositivePrice) {
  IexOrderFactory f;
  EXPECT_EQ(f.create_limit(1, Side::kBuy, 0, 100, TimeInForce::kDay, false),
            nullptr);
  EXPECT_EQ(f.create_limit(1, Side::kBuy, -1, 100, TimeInForce::kDay, false),
            nullptr);
  EXPECT_EQ(f.create_stop(1, Side::kBuy, 0, 100), nullptr);
}

TEST(FactoryOrdersTest, MarketOrderHasNoPrice) {
  IexOrderFactory f;
  auto o = f.create_market(4, Side::kSell, 500);
  ASSERT_NE(o, nullptr);  // stub: nullptr -> red
  EXPECT_EQ(o->kind, OrderKind::kMarket);
  EXPECT_EQ(o->price, 0);  // market prints carry no limit price
  EXPECT_EQ(o->qty, 500);
  EXPECT_EQ(o->side, Side::kSell);
  EXPECT_FALSE(o->post_only);
}

TEST(FactoryOrdersTest, StopOrderCarriesTriggerAsPrice) {
  CmeOrderFactory f;
  auto o = f.create_stop(9, Side::kBuy, kPrice12_344, 25);
  ASSERT_NE(o, nullptr);  // stub: nullptr -> red
  EXPECT_EQ(o->kind, OrderKind::kStop);
  EXPECT_EQ(o->price, 12'345'000);  // trigger quantized to the CME grid
  EXPECT_EQ(o->qty, 25);
  EXPECT_FALSE(o->post_only);
}

TEST(FactoryOrdersTest, FactorySelectedByVenue) {
  auto a = make_order_factory(Venue::kIex);
  auto b = make_order_factory(Venue::kCme);
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(a->venue(), Venue::kIex);
  EXPECT_EQ(b->venue(), Venue::kCme);

  // The factory-of-factories hands out objects with the venue's rounding.
  auto o = b->create_limit(1, Side::kBuy, kPrice12_345, 10, TimeInForce::kDay,
                           false);
  ASSERT_NE(o, nullptr);  // stub: nullptr -> red
  EXPECT_EQ(o->price, 12'345'000);
}

}  // namespace