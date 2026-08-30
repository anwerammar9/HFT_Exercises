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