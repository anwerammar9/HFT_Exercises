# Exercise 26 — Matching Engine (Task)

## Problem
A **price-time priority** limit-order book that returns every trade an incoming
order generates. Bids better = higher price; asks better = lower price; within
one price, FIFO by arrival. Fills happen **always at the resting order's
price**; leftover quantity rests at the aggressor's price.

## Requirements (what the tests check)
1. `add_order(order)` returns ALL trades the order generated, in fill order.
   A fully-filled order never rests. Order ids are assumed unique (caller's
   responsibility).
2. Priority is by the order `add_order()` calls happen, NOT by `OrderId` value.
3. Equal-size crossing → exactly one trade at the **resting** price; book empty
   after; the aggressor's price never sets the fill price.
4. Price priority across levels; FIFO within a price level.
5. One large aggressive order sweeps multiple levels **in one `add_order`
   call**, returning all trades in fill order.
6. A non-crossing order rests without generating trades. Any remaining
   aggressor quantity rests at the aggressor's price, at the **tail** of that
   level's queue.
7. `cancel_order(id)`: removes a resting order; `false` (no throw) for
   unknown/already-filled ids. Other orders' queues are unaffected.

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

## Design notes
Two price trees (`bids_` keyed by `std::greater` so `begin()` is the best bid;
`asks_` ascending); each level is a `std::deque<LevelEntry{id, qty}>` FIFO.
Sweep the opposite book from `begin()`: while qty remains and the level price
crosses, consume `min(qty, head.qty)` at the resting price, stay on the same
level until it drains, then advance. Record resting `Loc{side, price}` in
`index_` for cancel. Single-threaded.

## Files
- Stub: `src/matching_engine.cpp`
- Tests: `test/test_matching_engine.cpp`
- Reference: `SOLUTION.md`