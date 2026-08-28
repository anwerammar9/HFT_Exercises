#include "l2_order_book.h"

#include <stdexcept>

// TODO(anwer): implement the real book (see SOLUTION.md).
//
// Suggested shape:
//   apply_snapshot: clear both maps, insert every level, has_snapshot_ = true.
//   apply_update:   if (!has_snapshot_) return;      // the no-book contract
//                   if (qty == 0) erase(price); else map[price] = qty;
//   best_*:         empty side -> nullopt, else *begin().
//   checksum:       fold (price, qty) of every level, bids then asks, through
//                   a splitmix-style mixer starting from a nonzero seed.

void L2Book::apply_snapshot(std::vector<PriceLevel> /*bids*/,
                            std::vector<PriceLevel> /*asks*/) {
  throw std::logic_error("not implemented");  // stub
}

void L2Book::apply_update(Side /*side*/, Price /*price*/, Qty /*new_qty*/) {
  throw std::logic_error("not implemented");  // stub
}

std::optional<PriceLevel> L2Book::best_bid() const { return std::nullopt; }

std::optional<PriceLevel> L2Book::best_ask() const { return std::nullopt; }

std::uint64_t L2Book::checksum() const { return 0; }