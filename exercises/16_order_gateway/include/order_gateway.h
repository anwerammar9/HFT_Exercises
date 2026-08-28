#ifndef EXERCISE16_ORDER_GATEWAY_H_
#define EXERCISE16_ORDER_GATEWAY_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <unordered_map>

// Idempotent order gateway: client-order-id dedup with TTL eviction.
//
// Contract:
//   - submit(cid, order) returns true and forwards `order` to the sink ONLY
//     the first time a given ClientOrderId is seen within dedup_window.
//     Resubmissions inside the window return false and are NOT forwarded
//     again (exactly-one-win across threads).
//   - Once dedup_window elapses (per the injectable clock), the same id may be
//     reused for a NEW order and is forwarded again.
//   - Thread-safe: concurrent submit() calls for one id resolve to exactly one
//     success.
//   - Clock: submit() calls now() (ns; steady by default). Tests inject a mock
//     `std::atomic<std::uint64_t>` so the window can be advanced without real
//     sleeping.
//
// Implementation notes (reference solution):
//   - A time-bucketed hash set (bucket = now()/window; keep the current and the
//     previous bucket) gives amortized O(1) TTL eviction with no per-entry
//     timers. Chosen over an LRU (Exercise 09) because a dedup window has no
//     recency-order to exploit — entries just age out together.
//   - Exactly-one-win is the same check-then-act race family as Exercise 25:
//     gate the "seen" insertion with one atomic compare-exchange.
//
// TODO(anwer): implement the gateway (see SOLUTION.md). Stub: submit always
// returns false and NEVER forwards -> tests run RED.
using ClientOrderId = std::uint64_t;
using Price = std::int64_t;
using OrderQty = std::int64_t;

enum class OrderSide { Buy, Sell };

struct Order {  // gateway view of an order; the id is the submit() key
  OrderSide side;
  Price price;
  OrderQty qty;
};

class OrderGateway {
 public:
  using Sink = std::function<void(const Order&)>;
  using NowFn = std::function<std::uint64_t()>;

  OrderGateway(std::chrono::nanoseconds dedup_window, Sink sink,
               NowFn now = NowFn(&OrderGateway::DefaultClock));

  bool submit(ClientOrderId cid, const Order& order);

 private:
  static std::uint64_t DefaultClock();

  std::chrono::nanoseconds dedup_window_;
  Sink sink_;
  NowFn now_;
};

#endif  // EXERCISE16_ORDER_GATEWAY_H_