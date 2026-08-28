# Exercise 19 — Limit Order Book (Task)

## Problem
Resting limit orders on two sides with **price-time priority** (FIFO within a
price), plus an aggressive `market_order` path that crosses the book best-first.

## Requirements (what the tests check)
1. `add_order(id, side, price, qty)` rests on the book; returns `false` if an
   order with the same id already exists.
2. **Price priority**: a buy crosses the better (lower) offers first. **Time
   priority**: at one price, the earlier order fills first (FIFO).
3. Each fill happens **at the resting order's price** (never the aggressor's).
4. A partial fill leaves the remainder resting at the same price and still
   first in its FIFO queue.
5. `market_order(side, qty)`: aggressive, sweeps levels best-first until qty is
   exhausted or that side of the book drains; never rests. Returns the fills in
   execution order plus the unmatched `remaining_qty`.
6. `cancel_order(id)`: `true` and removes the order; `false` if unknown.
7. `modify_order(id, new_qty)`: changes quantity only — **id, price AND time
   priority are preserved** (the order keeps its original queue position);
   `false` if unknown.
8. `best_bid()`/`best_ask()`: highest buy / lowest sell with the aggregate
   quantity at that price; `nullopt` when empty.
9. `qty_at(side, price)`: aggregate quantity across orders at a price;
   `order_count()`; `clear()`.

## Public API
```cpp
enum class Side { kBuy, kSell };
struct Tick { Side side; double price; std::uint64_t qty; };
struct Trade { std::uint64_t buy_order; std::uint64_t sell_order;
               double price; std::uint64_t qty; };
struct FillResult { std::vector<Trade> trades; std::uint64_t remaining_qty; };

class OrderBook {
  bool add_order(std::uint64_t id, Side side, double price, std::uint64_t qty);
  bool cancel_order(std::uint64_t id);
  bool modify_order(std::uint64_t id, std::uint64_t new_qty);
  FillResult market_order(Side side, std::uint64_t qty);
  std::optional<Tick> best_bid() const;
  std::optional<Tick> best_ask() const;
  std::optional<std::uint64_t> qty_at(Side side, double price) const;
  std::size_t order_count() const;
  void clear();
};
```

## Design notes
`by_id_` (id → Order with its queue iterator) + two `std::map<double,
std::list<uint64_t>>` price→FIFO-of-ids. Single-threaded by design; all prices
compare exactly.

## Files
- Stub: `src/order_book.cpp`
- Tests: `test/test_order_book.cpp`
- Reference: `SOLUTION.md`