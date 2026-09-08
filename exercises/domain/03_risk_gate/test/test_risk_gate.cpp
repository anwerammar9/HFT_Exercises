#include "risk_gate.h"

#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace {

Order make_order(std::int64_t id, Side side, std::int64_t price,
                 std::int64_t qty) {
  return Order{id, side, price, qty};
}

TEST(RiskGateTest, OrderWithinLimitsAcceptedAndReserves) {
  RiskGate gate(1000, 100000);
  Order buy = make_order(1, Side::Buy, 10, 100);
  EXPECT_TRUE(gate.check_and_reserve(buy));  // stub: false -> red
  EXPECT_EQ(gate.position(), 100);
  EXPECT_EQ(gate.notional_used(), 1000);
}

TEST(RiskGateTest, PositionLimitBreachRejectedAndReservesNothing) {
  RiskGate gate(100, 100000);
  Order big = make_order(1, Side::Buy, 10, 101);
  EXPECT_FALSE(gate.check_and_reserve(big));
  EXPECT_EQ(gate.position(), 0);
  EXPECT_EQ(gate.notional_used(), 0);
}

TEST(RiskGateTest, NotionalLimitBreachRejected) {
  RiskGate gate(1000, 1000);
  Order big = make_order(1, Side::Buy, 10, 101);  // notional 1010 > 1000
  EXPECT_FALSE(gate.check_and_reserve(big));
  EXPECT_EQ(gate.position(), 0);
}

TEST(RiskGateTest, BoundaryExactlyAtLimitAccepted) {
  RiskGate gate(100, 10000);
  Order exact = make_order(1, Side::Buy, 100, 100);  // notional exactly 10000
  EXPECT_TRUE(gate.check_and_reserve(exact));        // stub: red here
  Order one_over = make_order(2, Side::Buy, 100, 1); // +qty breaches position
  EXPECT_FALSE(gate.check_and_reserve(one_over));
}

TEST(RiskGateTest, SellReducesPositionAndAccruesGrossNotional) {
  RiskGate gate(1000, 100000);
  Order buy = make_order(1, Side::Buy, 10, 300);
  Order sell = make_order(2, Side::Sell, 10, 100);
  ASSERT_TRUE(gate.check_and_reserve(buy));
  EXPECT_TRUE(gate.check_and_reserve(sell));  // net 200 <= 1000
  EXPECT_EQ(gate.position(), 200);
  EXPECT_EQ(gate.notional_used(), 4000);      // gross: 3000 + 1000
}

TEST(RiskGateTest, ReleaseFreesCapacity) {
  RiskGate gate(100, 100000);
  Order a = make_order(1, Side::Buy, 10, 100);
  ASSERT_TRUE(gate.check_and_reserve(a));  // at the position limit
  Order b = make_order(2, Side::Buy, 10, 1);
  EXPECT_FALSE(gate.check_and_reserve(b));
  gate.release(a);                        // cancel/reject frees the slot
  EXPECT_TRUE(gate.check_and_reserve(b));
  EXPECT_EQ(gate.position(), 1);
}

TEST(RiskGateTest, ConcurrentPositionLimitNeverBreached) {
  constexpr std::int64_t kLimit = 10000;
  constexpr int kThreads = 8;
  std::int64_t quota = kLimit / kThreads;  // 8 * 1250 == 10000 exactly

  RiskGate gate(kLimit, 100000000);
  std::atomic<int> accepted{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&, t] {
      Order o = make_order(1000 + t, Side::Buy, 1, quota);
      for (int attempt = 0; attempt < 20000; ++attempt) {
        if (gate.check_and_reserve(o)) {
          accepted.fetch_add(1);
          break;
        }
        if ((attempt & 255) == 255) std::this_thread::yield();
      }
    });
  }
  for (auto& th : threads) th.join();

  EXPECT_EQ(accepted.load(), kThreads);  // stub: 0 -> red
  EXPECT_EQ(gate.position(), kLimit);    // never overshot the limit
}

TEST(RiskGateTest, ConcurrentNotionalLimitNeverBreached) {
  constexpr std::int64_t kNotionalLimit = 80000;
  constexpr int kThreads = 8;

  RiskGate gate(1000000, kNotionalLimit);
  std::atomic<int> accepted{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&, t] {
      Order o = make_order(2000 + t, Side::Buy, 10, 1000);  // notional 10000 each
      for (int attempt = 0; attempt < 20000; ++attempt) {
        if (gate.check_and_reserve(o)) {
          accepted.fetch_add(1);
          break;
        }
        if ((attempt & 255) == 255) std::this_thread::yield();
      }
    });
  }
  for (auto& th : threads) th.join();

  EXPECT_EQ(accepted.load(), kThreads);          // stub: 0 -> red
  EXPECT_EQ(gate.notional_used(), kNotionalLimit);  // 8 * 10000 == limit
}

}  // namespace