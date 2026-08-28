# Exercise 18 — L2 Order Book (Task)

## Problem
A market-data book builder that merges incremental feed updates on top of an
initial snapshot, keeps each side sorted best-first, and exposes a
deterministic **checksum** for the exchange "resync on mismatch" pattern.

## Requirements (what the tests check)
1. `apply_snapshot(bids, asks)` replaces the ENTIRE book. Each vector arrives
   **best-first** from the feed (highest bid / lowest ask first).
2. `apply_update(side, price, new_qty)` sets that price level's quantity;
   `new_qty == 0` **removes** the level (removing the best naturally exposes
   the next-best).
3. Updates received **before any snapshot** are ignored (documented no-op
   contract).
4. `best_bid()` / `best_ask()` return the highest bid / lowest ask level, or
   `nullopt` when that side is empty.
5. `checksum()` is deterministic: the same op sequence always yields the same
   digest; a qty change changes it; the **empty book hashes to 0**.

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

## Design notes
One sorted `std::map` per side (bids descending via `std::greater`, asks
ascending) so `best_*` is `*begin()`. Checksum: fold (price, qty) of every
level, bids then asks, through a mixing hash (e.g. splitmix64-style), seeded
with a nonzero constant so a nonempty book can't collide with the empty-book 0.

## Files
- Stub: `src/l2_order_book.cpp`
- Tests: `test/test_l2_order_book.cpp`
- Reference: `SOLUTION.md`