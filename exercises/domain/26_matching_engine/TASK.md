# Exercise 26 — Matching Engine (Task)

## The problem (in plain words)

The capstone: a **price-time priority limit-order book** with aggressive order
crossing. Bids are better when the price is *higher*, asks better when *lower*
— and every fill is priced at the **resting order's price**, never the
aggressor's. One large aggressive order can sweep multiple levels **in a single
`add_order`** call, and every trade it generates must be returned, in fill
order.

## Requirements (what the tests check)

1. `add_order(order)` returns **every trade** the order generated, in fill
   order. A fully-filled order never rests. Order ids are assumed unique (the
   caller's responsibility).
2. Time priority is by `add_order()` call order, **NOT** by `OrderId` value.
3. An equal-size crossing produces exactly one trade at the **resting** price;
   the book is empty afterwards; the aggressor's price never sets the fill
   price.
4. Price priority across levels; FIFO within a price level.
5. One large aggressive order **sweeps multiple levels in one call**, returning
   all trades in fill order.
6. A non-crossing order rests without generating trades. Leftover aggressor
   quantity rests at the **aggressor's price**, at the **tail** of that level's
   queue.
7. `cancel_order(id)`: removes a resting order; `false` (no throw) for
   unknown or already-filled ids. Other orders' queues are unaffected.

## Public API

```cpp
using OrderId = std::uint64_t;
using Price = std::int64_t;
using Qty = std::int64_t;
enum class Side : std::uint8_t { Buy, Sell };

struct Order { OrderId id; Side side; Price price; Qty qty; };

struct Trade {
  OrderId resting_id;     // the order already in the book
  OrderId aggressor_id;   // the order that just arrived and crossed
  Price price;            // ALWAYS the resting order's price
  Qty qty;
};

class MatchingEngine {
  std::vector<Trade> add_order(Order order);
  bool cancel_order(OrderId id);
};
```

## How to think about it (suggested design)

- Two price trees: `bids_` keyed by `std::greater<Price>` (so `begin()` is the
  best bid) and `asks_` ascending. Each level is a `std::deque<LevelEntry{id,
  qty}>` — the FIFO for time priority.
- `add_order` does one sweep of the **opposite** book:
  1. while aggressor qty remains and the opposite side's best level price
     *crosses* (buy: `best_ask <= price`; sell: `best_bid >= price`):
  2. consume `min(aggr_remaining, head.qty)` at the **resting** price, stay on
     the same level until it drains, then advance to the next;
  3. record a trade each time.
- Any leftover aggressor quantity rests by pushing onto its own side's level
  queue (tail). Keep an `id → {side, price}` map (`index_`) so `cancel_order`
  is a direct lookup → remove-from-deque.
- The header reserves these members; the exercise is the *loop discipline* of
  the sweep, and making sure the resting-price rule holds on every fill.
- Single-threaded, like the real pre-trade path.

## Make it harder (optional — not covered by the tests)

- **Aggressive limit + stop the sweep:** a crossing limit order that stops
  sweeping at its own limit price (rests the rest at its limit) — the
  difference between a matching engine and a real one.
- **`modify_order`:** change quantity with time-priority preserved (the
  Exercise 19 trick), and reprice variants with explicit priority rules.
- **GTT expiry:** timestamps + a `expire_now(now)` that cancels every resting
  order past its Good-Till-Time, driven by Exercise 10's wheel.
- **FOK/IOC/GTX flags:** execute-in-full-or-cancel, immediate-or-cancel, and
  post-only-reject-quantity → return the remained `Order` back tagged as
  rejected.
- **Randomized differential test:** a reference `std::map`-based naive
  matcher + a scripted generator; assert your engine's fills (ids, prices,
  qtys) match the reference exactly across thousands of scenarios.

## Files

- Stub: `src/matching_engine.cpp`
- Tests: `test/test_matching_engine.cpp`
- Reference: `SOLUTION.md`