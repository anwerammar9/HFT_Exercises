# Exercise domain/04_l2_order_book (ex18) — L2 Order Book (Reference Solution)

**What you implement:** a market-data book builder that merges incremental
feed updates on top of an initial snapshot, keeps each side sorted
best-first, and exposes a deterministic checksum for the exchange "resync on
mismatch" pattern.

**Approach**
- Two `std::map<Price, Qty>`s sorted best-first: bids `less`-reversed via
  `std::greater` (highest bid at `begin()`), asks ascending (`ascending`), so
  `best_bid`/`best_ask` are O(1) `*begin()`.
- `apply_snapshot` replaces the book wholesale and flips `has_snapshot_`.
- `apply_update`: **no-op until the first snapshot** (the chosen contract),
  then insert/update a price (`qty=0` erases the level — removing the best
  naturally exposes the next-best).
- `checksum`: fold `(price, qty)` of every level, bids then asks, through a
  splitmix64 finalizer from a nonzero seed; empty book → **0**. Deterministic
  by construction — two identically-sequenced books agree, and a qty change
  changes the digest.

## Reference API — `include/l2_order_book.h`
#ifndef EXERCISE18_L2_ORDER_BOOK_H_
#define EXERCISE18_L2_ORDER_BOOK_H_

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <vector>

// L2 order-book builder: snapshot + incremental-update merger (market data).
//
// Contract:
//   - apply_snapshot(bids, asks) replaces the ENTIRE book. Each vector arrives
//     best-first from the feed (highest bid, lowest ask first).
//   - apply_update(side, price, new_qty) sets that price level's qty; qty == 0
//     REMOVES the level. Updates received before any snapshot are IGNORED
//     (no-op) — the documented no-book-yet contract.
//   - best_bid() / best_ask(): the highest bid / lowest ask level, or nullopt
//     when that side is empty.
//   - checksum(): a deterministic digest of the book content — the same op
//     sequence always yields the same checksum (the exchange "resync on
//     checksum mismatch" pattern). Empty book hashes to 0.
//
// Implementation notes (reference solution):
//   - One sorted-by-price `std::map` per side, ordered best-first (bids
//     descending, asks ascending via comparator) so best_* is O(1) (*begin()).
//   - checksum: fold (price, qty) of every level, bids then asks, through a
//     mixing hash (e.g. splitmix64-style) seeded with a nonzero constant so a
//     nonempty book can't collide with the empty-book 0.
//
// TODO(anwer): implement the book (see SOLUTION.md). Stub: snapshot/update
// throw, best_* is nullopt, checksum is 0 -> tests run RED.
using Price = std::int64_t;
using Qty = std::int64_t;

enum class Side { Bid, Ask };

struct PriceLevel {
  Price price;
  Qty qty;
};

class L2Book {
 public:
  void apply_snapshot(std::vector<PriceLevel> bids, std::vector<PriceLevel> asks);
  void apply_update(Side side, Price price, Qty new_qty);

  std::optional<PriceLevel> best_bid() const;
  std::optional<PriceLevel> best_ask() const;

  std::uint64_t checksum() const;

 private:
  bool has_snapshot_{false};
  std::map<Price, Qty, std::greater<Price>> bids_;
  std::map<Price, Qty> asks_;
};

#endif  // EXERCISE18_L2_ORDER_BOOK_H_

## Reference implementation — `src/l2_order_book.cpp`
#include "l2_order_book.h"

#include <utility>

namespace {

// splitmix64 finalizer — a cheap bijection for the checksum fold.
std::uint64_t mix(std::uint64_t h) {
  h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ull;
  h = (h ^ (h >> 27)) * 0x94d049bb133111ebull;
  return h ^ (h >> 31);
}

}  // namespace

void L2Book::apply_snapshot(std::vector<PriceLevel> bids,
                            std::vector<PriceLevel> asks) {
  bids_.clear();
  asks_.clear();
  for (const PriceLevel& l : bids) bids_.emplace(l.price, l.qty);
  for (const PriceLevel& l : asks) asks_.emplace(l.price, l.qty);
  has_snapshot_ = true;
}

void L2Book::apply_update(Side side, Price price, Qty new_qty) {
  if (!has_snapshot_) return;  // the no-book-yet contract
  if (side == Side::Bid) {
    if (new_qty == 0)
      bids_.erase(price);
    else
      bids_[price] = new_qty;
  } else {
    if (new_qty == 0)
      asks_.erase(price);
    else
      asks_[price] = new_qty;
  }
}

std::optional<PriceLevel> L2Book::best_bid() const {
  if (bids_.empty()) return std::nullopt;
  auto it = bids_.begin();  // map<Price, Qty, greater<>>: best bid first
  return PriceLevel{it->first, it->second};
}

std::optional<PriceLevel> L2Book::best_ask() const {
  if (asks_.empty()) return std::nullopt;
  auto it = asks_.begin();  // map<Price, Qty>: best ask (lowest) first
  return PriceLevel{it->first, it->second};
}

std::uint64_t L2Book::checksum() const {
  if (bids_.empty() && asks_.empty()) return 0;  // empty book hashes to 0
  std::uint64_t h = 0x9E3779B97F4A7C15ull;       // nonzero seed
  for (const auto& [price, qty] : bids_) {
    h = mix(h ^ (static_cast<std::uint64_t>(price) * 0xC6A4A7935BD1E995ull) ^
                 (static_cast<std::uint64_t>(qty) + 0x13198A2Eull));
  }
  for (const auto& [price, qty] : asks_) {
    h = mix(h ^ (static_cast<std::uint64_t>(qty) * 0xC6A4A7935BD1E995ull) ^
                 (static_cast<std::uint64_t>(price) + 0x165667B1ull));
  }
  return h;
}