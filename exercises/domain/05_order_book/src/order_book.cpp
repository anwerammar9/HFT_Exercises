#include "order_book.h"

// TODO(anwer): implement the limit order book (see SOLUTION.md).
//
// Suggested shape: `by_id_` (id -> Order{side, price, qty, queue_pos}) + a
// `std::map<double, std::list<id>>` per side as the price-time FIFO queue.
//   - add_order:     reject if the id exists; else push onto book[price] and
//                    remember the list iterator.
//   - cancel_order:  erase from by_id_ + splice out of its price queue; drop
//                    empty levels.
//   - modify_order:  change qty only (price & time priority preserved).
//   - market_order:  consume the opposite book best-first until qty exhausted;
//                    fill qty at the RESTING price, flatten consumed orders.
//   - best_bid/ask:  last/first level with aggregate qty; nullopt if empty.
//
// Stub: an always-empty book — adds are rejected and reads return nullopt, so
// the tests run RED with zero risk of OOB/crash.

bool OrderBook::add_order(std::uint64_t /*id*/, Side /*side*/, double /*price*/,
                          std::uint64_t /*qty*/) {
  return false;
}

bool OrderBook::cancel_order(std::uint64_t /*id*/) { return false; }

bool OrderBook::modify_order(std::uint64_t /*id*/, std::uint64_t /*new_qty*/) {
  return false;
}

FillResult OrderBook::market_order(Side /*side*/, std::uint64_t /*qty*/) {
  return FillResult{};
}

std::optional<Tick> OrderBook::best_bid() const { return std::nullopt; }

std::optional<Tick> OrderBook::best_ask() const { return std::nullopt; }

std::optional<std::uint64_t> OrderBook::qty_at(Side /*side*/, double /*price*/) const {
  return std::nullopt;
}

std::size_t OrderBook::order_count() const { return by_id_.size(); }

void OrderBook::clear() {
  by_id_.clear();
  bids_.clear();
  asks_.clear();
}