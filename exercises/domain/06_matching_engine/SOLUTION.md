# Exercise domain/06_matching_engine (ex26) — Matching Engine (Reference Solution)

**What you implement:** a price-time priority limit-order book that returns the
trades each incoming order generates. Bids better = higher price; asks better =
lower price; within one price, FIFO by arrival. Fills ALWAYS at the resting
order's price; leftovers rest at the aggressor's price (at the level's tail).

**Approach**
- Resting side: two `std::map` price trees — bids keyed by `std::greater<Price>`
  so `begin()` is the best (highest) bid; asks ascending so `begin()` is the
  best (lowest) ask. Each level is a `std::deque<LevelEntry{id, qty}>` (FIFO).
- `add_order(Buy)`: sweep `asks_` `begin()`-up: while qty remains and the ask
  price ≤ buy price, consume the level head min(qty, head.qty), emitting
  `Trade{resting_id, aggressor_id, resting_price, fill}`, popping empty heads
  and erasing drained levels — staying on the SAME level until it's exhausted
  (correct FIFO priority), then advancing. Stop when `price > order.price`
  (would rest below/above). Any remaining qty rests at `order.price`, appended
  at the level tail (`push_back`), with its `Loc` recorded in `index_`.
- Sells mirror the sweep over `bids_`.
- `cancel_order(id)`: look up `index_`, linear-remove the id from its level's
  deque, drop empty levels, erase the index entry; false for unknown/filled.
- Single-threaded; ids are caller-unique.

## Reference API — `include/matching_engine.h`
#ifndef EXERCISE26_MATCHING_ENGINE_H_
#define EXERCISE26_MATCHING_ENGINE_H_

#include <cstdint>
#include <deque>
#include <map>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

// Price-time priority limit-order book.
//
// Types (from the plan):
using OrderId = std::uint64_t;
using Price = std::int64_t;  // signed: allow negatives/credits; unsigned is fine too
using Qty = std::int64_t;

enum class Side : std::uint8_t { Buy, Sell };

struct Order {
  OrderId id;
  Side side;
  Price price;
  Qty qty;
};

struct Trade {
  OrderId resting_id;     // the order already in the book
  OrderId aggressor_id;   // the order that just arrived and crossed
  Price price;            // always the RESTING order's price
  Qty qty;
};

// Price-time priority:
//   - Bids: higher price = better (fills first); at a given price FIFO by
//     arrival. Asks: lower price = better.
//   - An aggressive (incoming) order crosses the OPPOSITE book, best level
//     first, FIFO within each level. A fill always happens at the resting
//     order's price. Any remaining aggressor quantity rests at its own price,
//     at the TAIL of that level's queue.
//   - Priority is determined by the order in which add_order() calls happen,
//     NOT by the OrderId value.
//
// Contract:
//   - add_order(order): returns ALL trades this order generated, in fill order.
//     Fully-filled orders never rest. Order ids are assumed unique (caller's
//     responsibility).
//   - cancel_order(id): removes a resting order; only that order's own queue
//     is unaffected otherwise. False (no throw) for unknown/already-filled ids.
//   - Market orders were the plan's stretch goal — not in the base API.
//
// TODO(anwer): implement in src/matching_engine.cpp.
//   Suggested shape: two price-tree structures (bids / asks) where each price
//   level is a FIFO queue of (id, qty), plus an id->level map for cancel.
//   std::map<Price, ...> gives the sorted best-price iteration for an exchange
//   interview; a lock-free or pmr-backed version is the "next level" follow-up.

class MatchingEngine {
 public:
  MatchingEngine() = default;

  MatchingEngine(const MatchingEngine&) = delete;
  MatchingEngine& operator=(const MatchingEngine&) = delete;

  std::vector<Trade> add_order(Order order);

  bool cancel_order(OrderId id);

 private:
  struct LevelEntry {
    OrderId id;
    Qty qty;
  };
  // Bids: highest price first (std::greater => begin() is the best bid).
  std::map<Price, std::deque<LevelEntry>, std::greater<Price>> bids_;
  // Asks: lowest price first (begin() is the best ask).
  std::map<Price, std::deque<LevelEntry>> asks_;
  struct Loc {
    Side side;
    Price price;
  };
  std::unordered_map<OrderId, Loc> index_;
};

#endif  // EXERCISE26_MATCHING_ENGINE_H_
## Reference implementation — `src/matching_engine.cpp`
#include "matching_engine.h"

#include <algorithm>
#include <utility>

std::vector<Trade> MatchingEngine::add_order(Order order) {
  std::vector<Trade> trades;

  if (order.side == Side::Buy) {
    // Sweep the ask book: best (lowest) ask first, FIFO within a level.
    auto level = asks_.begin();
    while (order.qty > 0 && level != asks_.end()) {
      if (level->first > order.price) break;  // stops crossing -> rest below
      auto& queue = level->second;
      LevelEntry& head = queue.front();
      const Qty fill = std::min(order.qty, head.qty);
      trades.push_back(Trade{head.id, order.id, level->first, fill});
      head.qty -= fill;
      order.qty -= fill;
      if (head.qty == 0) queue.pop_front();
      if (queue.empty()) {
        level = asks_.erase(level);
      }
      // If the level still has orders and the aggressor remains, loop again on
      // the SAME level (continue consuming it) rather than moving on.
    }
    if (order.qty > 0) {  // remainder rests as a bid
      bids_[order.price].push_back(LevelEntry{order.id, order.qty});
      index_[order.id] = Loc{Side::Buy, order.price};
    }
  } else {
    // Sweep the bid book: best (highest) bid first, FIFO within a level.
    auto level = bids_.begin();
    while (order.qty > 0 && level != bids_.end()) {
      if (level->first < order.price) break;  // stops crossing -> rest above
      auto& queue = level->second;
      LevelEntry& head = queue.front();
      const Qty fill = std::min(order.qty, head.qty);
      trades.push_back(Trade{head.id, order.id, level->first, fill});
      head.qty -= fill;
      order.qty -= fill;
      if (head.qty == 0) queue.pop_front();
      if (queue.empty()) {
        level = bids_.erase(level);
      }
      // If the level still has orders and the aggressor remains, loop again on
      // the SAME level (continue consuming it) rather than moving on.
    }
    if (order.qty > 0) {  // remainder rests as an ask
      asks_[order.price].push_back(LevelEntry{order.id, order.qty});
      index_[order.id] = Loc{Side::Sell, order.price};
    }
  }

  return trades;
}

bool MatchingEngine::cancel_order(OrderId id) {
  auto idx = index_.find(id);
  if (idx == index_.end()) return false;
  const Loc loc = idx->second;
  index_.erase(idx);

  auto remove_from_level = [&](auto& book) {
    auto level = book.find(loc.price);
    if (level == book.end()) return false;
    auto& queue = level->second;
    for (auto it = queue.begin(); it != queue.end(); ++it) {
      if (it->id == id) {
        queue.erase(it);
        if (queue.empty()) book.erase(level);
        return true;
      }
    }
    return false;
  };

  return loc.side == Side::Buy ? remove_from_level(bids_) : remove_from_level(asks_);
}
