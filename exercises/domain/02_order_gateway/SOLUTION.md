# Exercise domain/02_order_gateway (ex16) — Idempotent Order Gateway (Reference Solution)

**What you implement:** an idempotence layer in front of a broker/OMS — a
client-order-id dedup window that forwards each id at most once, then lets the
id be *reused* once the window elapses, all thread-safe with an injectable
clock.

**Approach**
- Per-id timestamp map `seen_`: `submit()` records `now()` at first forward; a
  resubmission is a duplicate iff `now − seen_[cid] <= dedup_window`.
- **Lazy TTL:** `compact_locked(now)` erases entries older than the window. It
  runs at the top of every `submit`, so the map stays bounded by "ids seen in
  the last window" with no per-entry timers and no background thread. (The
  header's original spike hinted at a *time-bucketed* set — the timestamp map
  is simpler and exact, which matters because the reuse test advances the
  mock clock by only ~window+100001ns; bucket drift could hold an id hostage
  for another whole bucket.)
- **Exactly-one-win:** all state — lookup, dedup decision, upsert, and the
  sink forward — happens under one `std::mutex`; concurrent copies of the same
  id serialize and exactly one forwarded. This is the same check-then-act
  guarantee as Exercise 25/17, here with a lock instead of a CAS.
- The sink is invoked **while holding the lock**: that makes the "concurrent
  submits → exactly one forward" property airtight.
- Clock: `submit()` calls `now()` each time; tests inject a mock
  `std::atomic<std::uint64_t>` (steady by default) so windows can be shifted
  without real sleeping.

## Reference API — `include/order_gateway.h`
#ifndef EXERCISE16_ORDER_GATEWAY_H_
#define EXERCISE16_ORDER_GATEWAY_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
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
//   - Per-id timestamp map with LAZY TTL eviction (compact on submit) — no
//     per-entry timers, no background thread; the map is bounded by the ids
//     seen in one window.
//   - Exactly-one-win is the same check-then-act race family as Exercise 25:
//     the lookup, dedup decision, upsert and the sink forward all happen under
//     one mutex, so concurrent copies of one id serialize.
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

  // seen_[cid] holds the clock value of the first submit; a resubmit is a
  // duplicate while now - seen_[cid] <= dedup_window_. Entries older than the
  // window are dropped by compact_locked() (the map stays bounded by ids live
  // in the window).
  std::unordered_map<ClientOrderId, std::uint64_t> seen_;
  std::mutex mu_;
  void compact_locked(std::uint64_t now);
};

#endif  // EXERCISE16_ORDER_GATEWAY_H_

## Reference implementation — `src/order_gateway.cpp`
#include "order_gateway.h"

#include <chrono>

// Idempotent gateway: a per-id timestamp map with lazy TTL eviction. The
// whole decision is one mutex + map lookup — a resubmit inside the window is
// a duplicate and is dropped; once the window has fully elapsed (per the
// injectable clock) the id may be reused and the order is forwarded. The
// sink call happens while holding the lock, so concurrently arriving copies
// of one id serialize: exactly one wins.

namespace {
std::uint64_t to_ns(std::chrono::steady_clock::time_point t) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          t.time_since_epoch())
          .count());
}
}  // namespace

std::uint64_t OrderGateway::DefaultClock() {
  return to_ns(std::chrono::steady_clock::now());
}

OrderGateway::OrderGateway(std::chrono::nanoseconds dedup_window, Sink sink,
                           NowFn now)
    : dedup_window_(dedup_window), sink_(std::move(sink)),
      now_(std::move(now)) {
  if (dedup_window_ <= std::chrono::nanoseconds::zero()) {
    // An invalid window would reject nothing; treat as always-reuse instead of
    // the constructor silently misbehaving — belt and braces for the tests.
    dedup_window_ = std::chrono::nanoseconds::zero();
  }
}

void OrderGateway::compact_locked(std::uint64_t now) {
  for (auto it = seen_.begin(); it != seen_.end();) {
    const std::uint64_t age = now - it->second;
    if (age > static_cast<std::uint64_t>(dedup_window_.count())) {
      it = seen_.erase(it);
    } else {
      ++it;
    }
  }
}

bool OrderGateway::submit(ClientOrderId cid, const Order& order) {
  const std::uint64_t now = now_();
  std::lock_guard<std::mutex> g(mu_);
  compact_locked(now);

  auto it = seen_.find(cid);
  if (it != seen_.end()) {
    const std::uint64_t age = now - it->second;
    if (age <= static_cast<std::uint64_t>(dedup_window_.count())) {
      return false;  // duplicate inside the window: not forwarded
    }
    // Window elapsed: the id is reusable; refresh the stamp below.
  }

  seen_[cid] = now;
  sink_(order);
  return true;
}