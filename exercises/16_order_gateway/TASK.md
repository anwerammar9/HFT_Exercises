# Exercise 16 — Idempotent Order Gateway (Task)

## Problem
An idempotence layer in front of a broker/OMS: a client-order-id dedup window
forwards each id **at most once**, lets the id be **reused** once the window
elapses, is safe under concurrent submit, and takes an injectable clock.

## Requirements (what the tests check)
1. `submit(cid, order)` returns `true` and forwards `order` to the sink **only
   the first time** that `cid` is seen within `dedup_window`.
2. Resubmission inside the window returns `false` and is **NOT forwarded**
   again.
3. Once `dedup_window` elapses (per the injectable clock), the same `cid` may
   be reused for a NEW order and is forwarded again.
4. Thread-safe: concurrent `submit` calls for one id resolve to **exactly one**
   success (the check-then-act race — gate the "seen" state atomically, e.g. a
   CAS/lock).
5. Clock: `submit()` calls `now()` (ns; steady by default). Tests inject a mock
   `std::atomic<std::uint64_t>` clock so the window can be advanced without
   sleeping.

## Public API
```cpp
using ClientOrderId = std::uint64_t;
using Price = std::int64_t;
using OrderQty = std::int64_t;
enum class OrderSide { Buy, Sell };
struct Order { OrderSide side; Price price; OrderQty qty; };

class OrderGateway {
  using Sink = std::function<void(const Order&)>;
  using NowFn = std::function<std::uint64_t()>;

  OrderGateway(std::chrono::nanoseconds dedup_window, Sink sink,
               NowFn now = NowFn(&OrderGateway::DefaultClock));
  bool submit(ClientOrderId cid, const Order& order);
};
```

## Design notes
Time-bucketed set (`bucket = now()/window`; keep current + previous bucket)
gives amortized O(1) TTL eviction with no per-entry timers. The header's
impl notes also spell out how the delete/reuse windowing works.

## Files
- Stub: `src/order_gateway.cpp`
- Tests: `test/test_order_gateway.cpp`
- Reference: `SOLUTION.md`