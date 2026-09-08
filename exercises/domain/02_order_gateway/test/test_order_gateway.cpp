#include "order_gateway.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace {

using std::chrono::nanoseconds;

// Deterministic logical clock: the tests just bump `now` (no real sleeping).
struct MockClock {
  std::atomic<std::uint64_t> now{0};
  std::uint64_t operator()() const {
    return now.load(std::memory_order_relaxed);
  }
  void advance(nanoseconds ns) {
    now.fetch_add(static_cast<std::uint64_t>(ns.count()));
  }
};

// Mock downstream sink: counts forwards, remembers what was sent.
struct SinkRecorder {
  std::atomic<int> forwarded{0};
  std::mutex mu;
  std::vector<Order> orders;

  void operator()(const Order& order) {
    forwarded.fetch_add(1);
    std::lock_guard<std::mutex> g(mu);
    orders.push_back(order);
  }
};

Order make_order(OrderSide side, Price price, OrderQty qty) {
  return Order{side, price, qty};
}

TEST(OrderGatewayTest, FirstSubmissionForwardsOnce) {
  MockClock clock;
  SinkRecorder sink;
  OrderGateway gateway(nanoseconds(100000), std::ref(sink), std::ref(clock));

  Order o = make_order(OrderSide::Buy, 100, 10);
  EXPECT_TRUE(gateway.submit(1, o));  // stub: false -> red
  EXPECT_EQ(sink.forwarded.load(), 1);
  {
    std::lock_guard<std::mutex> g(sink.mu);
    ASSERT_EQ(sink.orders.size(), 1u);
    EXPECT_EQ(sink.orders[0].price, 100);
  }
}

TEST(OrderGatewayTest, DuplicateWithinWindowRejected) {
  MockClock clock;
  SinkRecorder sink;
  OrderGateway gateway(nanoseconds(100000), std::ref(sink), std::ref(clock));

  Order o = make_order(OrderSide::Buy, 100, 10);
  ASSERT_TRUE(gateway.submit(1, o));

  clock.advance(nanoseconds(50000));  // still inside the window
  EXPECT_FALSE(gateway.submit(1, o));  // treated as a duplicate
  EXPECT_EQ(sink.forwarded.load(), 1);
}

TEST(OrderGatewayTest, IdReusableAfterWindowElapses) {
  MockClock clock;
  SinkRecorder sink;
  OrderGateway gateway(nanoseconds(100000), std::ref(sink), std::ref(clock));

  Order o = make_order(OrderSide::Buy, 100, 10);
  ASSERT_TRUE(gateway.submit(1, o));

  clock.advance(nanoseconds(100001));  // window elapsed
  Order newer = make_order(OrderSide::Sell, 102, 7);
  EXPECT_TRUE(gateway.submit(1, newer));  // the id may be reused
  EXPECT_EQ(sink.forwarded.load(), 2);
  {
    std::lock_guard<std::mutex> g(sink.mu);
    ASSERT_EQ(sink.orders.size(), 2u);
    EXPECT_EQ(sink.orders[1].price, 102);
  }
}

TEST(OrderGatewayTest, ConcurrentSameIdExactlyOneWin) {
  MockClock clock;
  SinkRecorder sink;
  OrderGateway gateway(nanoseconds(100000), std::ref(sink), std::ref(clock));

  Order o = make_order(OrderSide::Buy, 100, 10);
  std::atomic<int> wins{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t) {
    threads.emplace_back([&] {
      for (int attempt = 0; attempt < 2000 && wins.load() < 1; ++attempt) {
        if (gateway.submit(1, o)) wins.fetch_add(1);
      }
    });
  }
  for (auto& th : threads) th.join();

  EXPECT_EQ(wins.load(), 1);  // stub: 0 -> red
  EXPECT_EQ(sink.forwarded.load(), 1);
}

}  // namespace