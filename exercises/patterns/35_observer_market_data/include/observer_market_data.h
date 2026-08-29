#ifndef EXERCISE35_OBSERVER_MARKET_DATA_H_
#define EXERCISE35_OBSERVER_MARKET_DATA_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

// Observer pattern: a market-data feed fans synchronous events out to
// subscribers without knowing what they are.
//
// Contract:
//   - subscribe(observer, symbol) registers an observer and returns a unique
//     1-based ObserverId. The observer is notified only about `symbol` events,
//     or about everything when symbol == kAnySymbol.
//   - unsubscribe(id) removes a subscriber (returns false for an unknown or
//     already-removed id). Dropped observers are no longer notified and are
//     released (shared_ptr dropped) so they may be destroyed.
//   - publish_quote / publish_trade invoke on_quote / on_trade on a CONSISTENT
//     snapshot of the current subscribers, in subscription order. A
//     subscription made DURING a publish is not notified by that same publish.
//   - Thread-safe fan-out: the snapshot is taken under the lock and callbacks
//     run OUTSIDE it, so subscribers may (un)subscribe/re-subscribe from a
//     callback. A throwing observer is dropped by the feed (and any remaining
//     subscribers still receive the event).
//
// Single-writer-per-symbol by design in the tests; seq numbers ride on events.
//
// TODO(anwer): implement (see SOLUTION.md). Stub: subscribe returns the
// invalid id 0, unsubscribe false, subscriber_count 0 and publish is a no-op
// -> the fan-out tests run RED.

using ObserverId = std::uint64_t;
using SymbolId = std::uint64_t;
using PriceMicros = std::int64_t;

inline constexpr SymbolId kAnySymbol = 0;

struct Quote {
  SymbolId symbol = 0;
  PriceMicros bid = 0;
  std::int64_t bid_size = 0;
  PriceMicros ask = 0;
  std::int64_t ask_size = 0;
  std::uint64_t seq = 0;
};

struct Trade {
  SymbolId symbol = 0;
  PriceMicros price = 0;
  std::int64_t qty = 0;
  std::uint64_t seq = 0;
};

class MarketDataObserver {
 public:
  virtual ~MarketDataObserver() = default;
  virtual void on_quote(const Quote&) = 0;
  virtual void on_trade(const Trade&) = 0;
};

class MarketDataFeed {
 public:
  MarketDataFeed() = default;
  MarketDataFeed(const MarketDataFeed&) = delete;
  MarketDataFeed& operator=(const MarketDataFeed&) = delete;

  ObserverId subscribe(std::shared_ptr<MarketDataObserver> observer,
                       SymbolId symbol = kAnySymbol);
  bool unsubscribe(ObserverId id);

  std::size_t subscriber_count() const;

  void publish_quote(const Quote& quote);
  void publish_trade(const Trade& trade);

 private:
  struct Entry {
    std::shared_ptr<MarketDataObserver> observer;
    SymbolId symbol;
  };
  struct Slot {
    ObserverId id = 0;
    Entry entry;
  };

  mutable std::mutex mtx_;
  std::vector<Slot> slots_;
  ObserverId next_id_ = 1;
};

#endif  // EXERCISE35_OBSERVER_MARKET_DATA_H_