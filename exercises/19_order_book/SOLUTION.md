# Exercise 19 — Limit Order Book (Reference Solution)

**What you implement:** resting limit orders on two sides, price-time priority
(FIFO within a price), and an aggressive `market_order` path that crosses the
book best-price-first.

**Approach**
- Resting orders: `by_id_` (id → `{side, price, qty, queue_pos}`) plus
  `Book = std::map<double, std::list<id>>` for each side — buy side ascending,
  sell side ascending; a price level's list is the FIFO queue.
- `add_order`: reject if the id exists; else push the id onto
  `book[price]` (append = later = lower priority) and store the list iterator.
- `cancel_order`: erase from `by_id_`, splice out of its price queue, drop the
  level when empty. `modify_order`: change `qty` only — id/price/time priority
  untouched.
- `market_order(side, qty)`: aggressor walks the opposite book best-first
  (buys descend `asks_` from `begin()`; sells walk `bids_` via `rbegin()`),
  filling min(want, resting.qty) per order at the resting price, flattening
  fully-consumed orders, and erasing drained levels (with reverse-iterator-safe
  erase via `std::prev(it.base())`). Remaining aggressor qty = `remaining_qty`.
- `best_bid()` = last bid level; `best_ask()` = first ask level, each with the
  aggregate `qty_at` across the level's queue; `nullopt` when the side is
  empty. `clear()` empties everything.

## Reference API — `include/order_book.h`
#ifndef EXERCISE19_ORDER_BOOK_H_
#define EXERCISE19_ORDER_BOOK_H_

#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

// Limit order book: resting limit orders on two sides plus an aggressive
// market-order path that crosses the book.
//
// Contract:
//   - add_order(id, side, price, qty): rests on the book. Rejected (false) if
//     an order with the same id already exists.
//   - cancel_order(id): removes the order. false if unknown.
//   - modify_order(id, new_qty): changes quantity only; id, price AND time
//     priority are preserved (i.e. the order keeps its original queue
//     position). false if unknown.
//   - market_order(side, qty): aggressive. Executes against the best
//     opposite-price level(s), each fill at the resting price, sweeping
//     levels best-first until qty is exhausted or the book on that side is
//     drained. Never rests. Returns fills in execution order and the
//     unmatched remainder.
//   - best_bid()/best_ask(): highest buy / lowest sell with the aggregate
//     quantity at that single price. nullopt when that side is empty.
//   - qty_at(side, price): aggregate quantity across orders at a price.
//   - Price-time priority: at a given price, earlier (by add time) resting
//     orders fill first (FIFO).
//
// Single-threaded by design (matches its use in the matching engine); the
// concurrency worry is handled elsewhere. All prices compare exactly.

enum class Side { kBuy, kSell };

struct Tick {  // aggregate depth at one price on one side
  Side side;
  double price;
  std::uint64_t qty;
};

struct Trade {
  std::uint64_t buy_order;
  std::uint64_t sell_order;
  double price;
  std::uint64_t qty;
};

struct FillResult {
  std::vector<Trade> trades;    // in execution order
  std::uint64_t remaining_qty;  // aggressor qty that did not cross
};

class OrderBook {
 public:
  OrderBook() = default;

  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;
  OrderBook(OrderBook&&) = delete;
  OrderBook& operator=(OrderBook&&) = delete;

  bool add_order(std::uint64_t id, Side side, double price, std::uint64_t qty);
  bool cancel_order(std::uint64_t id);
  bool modify_order(std::uint64_t id, std::uint64_t new_qty);
  FillResult market_order(Side side, std::uint64_t qty);

  std::optional<Tick> best_bid() const;
  std::optional<Tick> best_ask() const;
  std::optional<std::uint64_t> qty_at(Side side, double price) const;
  std::size_t order_count() const;
  void clear();

 private:
  struct Order {
    Side side;
    double price;
    std::uint64_t qty;
    std::list<std::uint64_t>::iterator queue_pos;  // position in the price queue
  };
  using Book = std::map<double, std::list<std::uint64_t>>;

  std::uint64_t aggregate_qty(const std::list<std::uint64_t>& ids) const;

  std::unordered_map<std::uint64_t, Order> by_id_;
  Book bids_;  // price -> FIFO queue of order ids
  Book asks_;
};

#endif  // EXERCISE19_ORDER_BOOK_H_
## Reference implementation — `src/order_book.cpp`
#include "order_book.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

std::uint64_t OrderBook::aggregate_qty(
    const std::list<std::uint64_t>& ids) const {
  std::uint64_t total = 0;
  for (std::uint64_t id : ids) {
    auto it = by_id_.find(id);
    if (it != by_id_.end()) total += it->second.qty;
  }
  return total;
}

bool OrderBook::add_order(std::uint64_t id, Side side, double price,
                          std::uint64_t qty) {
  if (by_id_.count(id) != 0) return false;

  Book& book = side == Side::kBuy ? bids_ : asks_;
  std::list<std::uint64_t>& queue = book[price];
  queue.push_back(id);  // appended = later in time = lower priority (FIFO)
  by_id_.emplace(id, Order{side, price, qty, --queue.end()});
  return true;
}

bool OrderBook::cancel_order(std::uint64_t id) {
  auto it = by_id_.find(id);
  if (it == by_id_.end()) return false;

  Book& book = it->second.side == Side::kBuy ? bids_ : asks_;
  auto level = book.find(it->second.price);
  std::list<std::uint64_t>& queue = level->second;
  queue.erase(it->second.queue_pos);
  if (queue.empty()) book.erase(level);
  by_id_.erase(it);
  return true;
}

bool OrderBook::modify_order(std::uint64_t id, std::uint64_t new_qty) {
  auto it = by_id_.find(id);
  if (it == by_id_.end()) return false;
  it->second.qty = new_qty;  // id, price and time priority unchanged
  return true;
}

FillResult OrderBook::market_order(Side side, std::uint64_t qty) {
  FillResult res;
  std::uint64_t want = qty;

  if (side == Side::kBuy) {
    // aggressor buys: consume the sell side, lowest ask first.
    for (auto it = asks_.begin(); it != asks_.end() && want != 0;) {
      auto& [price, queue] = *it;
      while (!queue.empty() && want != 0) {
        const std::uint64_t id = queue.front();
        auto oit = by_id_.find(id);
        if (oit == by_id_.end()) {  // defensive; cancels keep the table clean
          queue.pop_front();
          continue;
        }
        Order& resting = oit->second;
        const std::uint64_t take = std::min(want, resting.qty);
        res.trades.push_back(Trade{0, id, price, take});
        resting.qty -= take;
        want -= take;
        if (resting.qty == 0) {
          by_id_.erase(oit);
          queue.pop_front();
        }
      }
      if (queue.empty()) {
        it = asks_.erase(it);
      } else {
        ++it;
      }
    }
  } else {
    // aggressor sells: consume the buy side, highest bid first.
    for (auto it = bids_.rbegin(); it != bids_.rend() && want != 0;) {
      auto& [price, queue] = *it;
      while (!queue.empty() && want != 0) {
        const std::uint64_t id = queue.front();
        auto oit = by_id_.find(id);
        if (oit == by_id_.end()) {
          queue.pop_front();
          continue;
        }
        Order& resting = oit->second;
        const std::uint64_t take = std::min(want, resting.qty);
        res.trades.push_back(Trade{id, 0, price, take});
        resting.qty -= take;
        want -= take;
        if (resting.qty == 0) {
          by_id_.erase(oit);
          queue.pop_front();
        }
      }
      if (queue.empty()) {
        // Erase the current element from a reverse_iterator safely.
        auto victim = std::prev(it.base());
        auto next_in_forward = bids_.erase(victim);
        it = std::make_reverse_iterator(next_in_forward);
      } else {
        ++it;
      }
    }
  }

  res.remaining_qty = want;
  return res;
}

std::optional<Tick> OrderBook::best_bid() const {
  if (bids_.empty()) return std::nullopt;
  auto it = std::prev(bids_.end());
  return Tick{Side::kBuy, it->first, aggregate_qty(it->second)};
}

std::optional<Tick> OrderBook::best_ask() const {
  if (asks_.empty()) return std::nullopt;
  auto it = asks_.begin();
  return Tick{Side::kSell, it->first, aggregate_qty(it->second)};
}

std::optional<std::uint64_t> OrderBook::qty_at(Side side, double price) const {
  const Book& book = side == Side::kBuy ? bids_ : asks_;
  auto it = book.find(price);
  if (it == book.end()) return std::nullopt;
  return aggregate_qty(it->second);
}

std::size_t OrderBook::order_count() const {
  return by_id_.size();
}

void OrderBook::clear() {
  by_id_.clear();
  bids_.clear();
  asks_.clear();
}