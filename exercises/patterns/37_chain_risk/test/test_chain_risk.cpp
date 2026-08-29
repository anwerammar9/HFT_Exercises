#include "chain_risk.h"

#include <memory>
#include <vector>

#include <gtest/gtest.h>

namespace {

// Test-only handler that records the requests it saw and always passes,
// proving whether the chain actually REACHED it (short-circuit probe).
class RecordingHandler : public RiskHandler {
 public:
  std::vector<std::uint64_t> seen;

 protected:
  CheckResult do_check(const OrderCheck& request) override {
    seen.push_back(request.id);
    return CheckResult{true, ""};
  }
};

OrderCheck order(std::uint64_t id, std::uint64_t symbol, PriceMicros price,
                 std::int64_t qty) {
  OrderCheck r;
  r.id = id;
  r.symbol = symbol;
  r.price = price;
  r.qty = qty;
  return r;
}

TEST(ChainRiskTest, EmptyChainAccepts) {
  RiskChain chain;
  auto r = chain.validate(order(1, 1, 1000, 10));
  EXPECT_TRUE(r.ok);  // stub: validate rejects -> red
  EXPECT_TRUE(r.reason.empty());
  EXPECT_EQ(chain.size(), 0u);
}

TEST(ChainRiskTest, SingleHandlerRejectsOverLimit) {
  RiskChain chain;
  chain.append(std::make_shared<LimitQtyHandler>(50));

  auto ok = chain.validate(order(1, 1, 1000, 30));
  EXPECT_TRUE(ok.ok);  // stub: rejects -> red

  auto bad = chain.validate(order(2, 1, 1000, 60));
  EXPECT_FALSE(bad.ok);
  EXPECT_EQ(bad.reason, "qty limit");  // stub: "not implemented" -> red
  EXPECT_EQ(chain.size(), 1u);
}

TEST(ChainRiskTest, ChainShortCircuitsOnFirstReject) {
  RiskChain chain;
  chain.append(std::make_shared<LimitQtyHandler>(50));
  auto probe = std::make_shared<RecordingHandler>();
  chain.append(probe);

  auto r = chain.validate(order(9, 1, 1000, 60));
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.reason, "qty limit");  // stub: "not implemented" -> red
  EXPECT_TRUE(probe->seen.empty());  // later handlers never ran
}

TEST(ChainRiskTest, PassesFlowThroughAllHandlers) {
  RiskChain chain;
  auto a = std::make_shared<RecordingHandler>();
  auto b = std::make_shared<RecordingHandler>();
  auto c = std::make_shared<RecordingHandler>();
  chain.append(a);
  chain.append(b);
  chain.append(c);

  auto r = chain.validate(order(3, 1, 1000, 10));
  EXPECT_TRUE(r.ok);  // stub: rejects -> red
  ASSERT_EQ(a->seen.size(), 1u);  // stub: never reached -> red
  ASSERT_EQ(b->seen.size(), 1u);
  ASSERT_EQ(c->seen.size(), 1u);
}

TEST(ChainRiskTest, HandlerOrderDeterminesReason) {
  RiskChain qty_first;
  qty_first.append(std::make_shared<LimitQtyHandler>(50));
  qty_first.append(std::make_shared<LimitPriceHandler>(10'000));
  EXPECT_EQ(qty_first.validate(order(1, 1, 20'000, 60)).reason, "qty limit");

  RiskChain price_first;
  price_first.append(std::make_shared<LimitPriceHandler>(10'000));
  price_first.append(std::make_shared<LimitQtyHandler>(50));
  EXPECT_EQ(price_first.validate(order(1, 1, 20'000, 60)).reason, "price limit");
}

TEST(ChainRiskTest, AllowedSymbolsHandler) {
  AllowedSymbolsHandler h({1, 2});
  EXPECT_TRUE(h.handle(order(1, 1, 1000, 10)).ok);  // belong -> pass

  auto r = h.handle(order(2, 3, 1000, 10));
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.reason, "symbol not allowed: 3");

  h.allow_symbol(3);
  EXPECT_TRUE(h.handle(order(3, 3, 1000, 10)).ok);
}

TEST(ChainRiskTest, NotionalAccruesAcrossValidation) {
  MaxNotionalHandler h(1'000'000);
  OrderCheck o = order(1, 1, 1000, 100);  // notional delta 100 * 1000
  EXPECT_TRUE(h.handle(o).ok);            // stub: rejects, used() == 0 -> red
  EXPECT_EQ(h.used(), 100'000);

  OrderCheck o2 = order(2, 1, 1000, 900);  // reaches the cap exactly
  EXPECT_TRUE(h.handle(o2).ok);
  EXPECT_EQ(h.used(), 1'000'000);

  OrderCheck o3 = order(3, 1, 1000, 1);  // would exceed -> reject
  auto r = h.handle(o3);
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.reason, "notional limit");
  EXPECT_EQ(h.used(), 1'000'000);  // nothing accrued on the reject

  h.reset();
  EXPECT_EQ(h.used(), 0);
}

TEST(ChainRiskTest, NotionalHandlerInsideChain) {
  RiskChain chain;
  chain.append(std::make_shared<LimitQtyHandler>(1'000));
  chain.append(std::make_shared<MaxNotionalHandler>(60));

  EXPECT_TRUE(chain.validate(order(1, 1, 10, 5)).ok);   // 50 <= 60
  auto bad = chain.validate(order(2, 1, 10, 6));        // +60 would be 110
  EXPECT_FALSE(bad.ok);
  EXPECT_EQ(bad.reason, "notional limit");
  EXPECT_TRUE(chain.validate(order(3, 1, 10, 1)).ok);   // +10 -> exactly 60
}

}  // namespace