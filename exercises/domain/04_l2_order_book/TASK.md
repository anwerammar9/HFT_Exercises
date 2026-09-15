# Exercise domain/04_l2_order_book (ex18) — L2 Order Book (Task)

## The problem (in plain words)

A market-data consumer builds a Level-2 book from two kinds of feed messages: an
initial **snapshot** (full depth, best-first) and then **incremental updates**
(`three, 100.5, 2500` = "price 100.5 now has quantity 2500"). Each side stays
sorted best-first, and the book exposes a **deterministic checksum** so the
consumer can detect "my book drifted from the exchange's" and resync — the
standard exchange reconciliation pattern.

## Requirements (what the tests check)

1. `apply_snapshot(bids, asks)` **replaces the entire book**. Each vector
   arrives **best-first** from the feed (highest bid / lowest ask first).
2. `apply_update(side, price, new_qty)` sets that price level's quantity;
   `new_qty == 0` **removes** the level (removing the best level naturally
   exposes the next-best).
3. Updates received **before any snapshot** are ignored — a documented no-op
   (no book exists yet to update).
4. `best_bid()` / `best_ask()` return the highest bid / lowest ask level, or
   `nullopt` when that side is empty.
5. `checksum()` is deterministic: the same sequence of operations always
   produces the same digest; changing a quantity changes the digest; the
   **empty book hashes to 0**.

## Public API

```cpp
using Price = std::int64_t;
using Qty = std::int64_t;
enum class Side { Bid, Ask };
struct PriceLevel { Price price; Qty qty; };

class L2Book {
  void apply_snapshot(std::vector<PriceLevel> bids, std::vector<PriceLevel> asks);
  void apply_update(Side side, Price price, Qty new_qty);
  std::optional<PriceLevel> best_bid() const;
  std::optional<PriceLevel> best_ask() const;
  std::uint64_t checksum() const;
};
```

## How to think about it (suggested design)

- One `std::map` per side — bids with `std::greater<Price>` (descending) and
  asks ascending — so `best_*` is simply `*begin()` and levels are always
  price-sorted for free.
- `apply_update` is a plain map operation: `operator[]` then set-or-erase on
  `qty == 0`. `has_snapshot_` guards the before-snapshot no-op.
- `checksum()`: fold every `(price, qty)` level, bids then asks, through a
  mixing hash (a splitmix64-style or FNV-style mixer) **seeded with a nonzero
  constant** so a non-empty book can never collide with the empty-book `0`.
- Order the fold deterministically (best-first on both sides) so two books with
  identical contents checksum identically regardless of the order they were
  built in.

## Make it harder (optional — not covered by the tests)

- **Cross check:** reject (or self-repair) updates that would cross
  `bid >= ask` — the real feeds treat crossing as "book is corrupt, resync".
- **Depth queries:** `depth_of_price(side, p)` and `best_n(n)` returning the
  top-N levels — a `std::vector<PriceLevel>` many clients need.
- **Sequence-gap guard:** pairs with Exercise 06 — refuse updates whose
  sequence number isn't contiguous after the snapshot.
- **Trade fan-out:** a `last_trade()`/`vwap_executed()` feed built from the
  implied print when a level's qty drops.
- **Snapshot validation:** verify the snapshot itself is sorted and buy/ask
  sides don't overlap before adopting it.

## Files

- Stub: `src/l2_order_book.cpp`
- Tests: `test/test_l2_order_book.cpp`
- Reference: `SOLUTION.md`