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