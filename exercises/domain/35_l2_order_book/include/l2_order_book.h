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