# Exercise domain/05_order_book (ex19) — Limit Order Book (Task)

## The problem (in plain words)

Resting **limit orders** sit on two sides of a book. Matching uses
**price-time priority**: at a given price the earlier order fills first (FIFO),
and a price level that has better precedence fills before worse ones. An
aggressive **market order** sweeps the opposite book best-first, and every fill
is **priced at the resting order's price** — the aggressor's price never
matters. A market order never rests; any unmatched remainder is reported back.

## Requirements (what the tests check)

1. `add_order(id, side, price, qty)` rests the order on the book; returns
   `false` if an order with the same id already exists.
2. **Price priority:** a buy crosses the better (lower) offers first. **Time
   priority:** at one price the earlier order fills first (FIFO).
3. Every fill happens at the **resting order's price**, never the aggressor's.
4. A partial fill leaves the remainder resting at the same price, still first
   in its price's FIFO queue.
5. `market_order(side, qty)`: aggressive; sweeps levels best-first until qty is
   exhausted or that side of the book drains; **never rests**. Returns the fills
   in execution order plus the unmatched `remaining_qty`.
6. `cancel_order(id)`: `true` and removes the order; `false` if unknown.
7. `modify_order(id, new_qty)`: changes quantity only — **id, price AND time
   priority are preserved** (the order keeps its original queue position);
   `false` if unknown.
8. `best_bid()`/`best_ask()`: highest buy / lowest sell with the **aggregate**
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

## How to think about it (suggested design)

- Three structures: `by_id_` (id → the order **plus its queue iterator**, for
  O(1) cancel/modify) and one `std::map<double, std::list<uint64_t>>` per side
  (price → FIFO of order ids). The map keeps levels price-sorted for free; the
  list is the FIFO for time priority.
- `market_order`: walk the opposite map from `begin()`; for each level, pop
  ids off the FIFO's front, fill `min(remaining, order.qty)` at the *level's*
  price, until qty is exhausted or the side drains.
- `modify_order` is where the priority contract lives: update `qty` in
  `by_id_` **without touching the list** — the iterator still points into the
  same FIFO position, so time priority survives unchanged.
- All prices compare exactly (`double` from the API); single-threaded by
  design.

## Make it harder (optional — not covered by the tests)

- **Aggressive limit orders:** `add_order` that *crosses* when the incoming
  price is marketable — execute like a market order up to the crossing price and
  rest any remainder (the missing piece in most book implementations).
- **Order types:** Good-Till-Cancel / Day / Fill-or-Kill / Immediate-or-Cancel /
  Post-Only flags, and the reject rules that go with them.
- **Self-trade prevention:** reject (or reprice) an order that would match
  itself at the same price on the other side.
- **`market_order` with a price cap** (`marketable_limit`): sweep only levels
  at or better than a given worst price, then rest the remainder.
- **Deterministic random tests:** generate a random add/cancel/modify/market
  script, invariant-check `best_bid() < best_ask()`, aggregate qty is
  non-negative, and `order_count()` matches `by_id_.size()` after each step.

## Files

- Stub: `src/order_book.cpp`
- Tests: `test/test_order_book.cpp`
- Reference: `SOLUTION.md`