# Exercise patterns/03_observer_market_data (ex35) — Observer (Market-Data Fan-out) (Reference Solution)

**What you implement:** the Observer pattern behind a market-data feed — a
thread-safe `MarketDataFeed` that synchronously fans quotes/trades out to
symbol-filtered subscribers on a *consistent snapshot*, running callbacks
outside the lock, with a throwing subscriber evicted instead of poisoning the
round.

**Approach**
- The feed stores `std::vector<Slot>` where `Slot = {id, {observer, symbol}}`,
  guarded by one mutex; ids come from a monotonic counter starting at 1.
- `subscribe()` rejects a null observer (`return 0`) and otherwise appends a
  slot; `unsubscribe(id)` removes the matching slot (drops the `shared_ptr` so
  the observer can be destroyed) and returns `false` when nothing matched.
- The publish path is the whole design: **lock → copy the vector → unlock →
  run callbacks**. Because callbacks run without the lock, a subscriber can
  (un)subscribe or re-subscribe from inside its own callback without deadlock
  or a corrupted round; and because the loop runs over a *copy*, a
  mid-publish subscription is naturally deferred to the next round (snapshot
  semantics the tests assert).
- Symbol filtering compares a slot's `symbol` (or `kAnySymbol`) against the
  event's symbol before invoking the callback.
- Each callback is wrapped in try/catch; on throw, `unsubscribe(slot.id)` so
  one bad subscriber can't take down the fan-out — and the confirmation that
  the remaining subscribers still received the event is a test.

## Reference API — `include/observer_market_data.h`
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

## Reference implementation — `src/observer_market_data.cpp`
#include "observer_market_data.h"

ObserverId MarketDataFeed::subscribe(std::shared_ptr<MarketDataObserver> observer,
                                     SymbolId symbol) {
  if (!observer) return 0;  // an empty subscriber cannot be honored
  std::lock_guard<std::mutex> lock(mtx_);
  const ObserverId id = next_id_++;
  slots_.push_back(Slot{id, {std::move(observer), symbol}});
  return id;
}

bool MarketDataFeed::unsubscribe(ObserverId id) {
  std::lock_guard<std::mutex> lock(mtx_);
  for (auto it = slots_.begin(); it != slots_.end(); ++it) {
    if (it->id == id) {
      slots_.erase(it);  // drops the shared_ptr: observer may be destroyed
      return true;
    }
  }
  return false;
}

std::size_t MarketDataFeed::subscriber_count() const {
  std::lock_guard<std::mutex> lock(mtx_);
  return slots_.size();
}

// Both publish paths follow the same shape: snapshot the subscriber list under
// the lock, run callbacks OUTSIDE it so a callback may (un)subscribe freely,
// and drop any observer that throws so one bad subscriber can't poison others.
void MarketDataFeed::publish_quote(const Quote& quote) {
  std::vector<Slot> snapshot;
  {
    std::lock_guard<std::mutex> lock(mtx_);
    snapshot = slots_;
  }
  for (const Slot& slot : snapshot) {
    if (slot.entry.symbol != kAnySymbol && slot.entry.symbol != quote.symbol)
      continue;
    try {
      slot.entry.observer->on_quote(quote);
    } catch (...) {
      unsubscribe(slot.id);
    }
  }
}

void MarketDataFeed::publish_trade(const Trade& trade) {
  std::vector<Slot> snapshot;
  {
    std::lock_guard<std::mutex> lock(mtx_);
    snapshot = slots_;
  }
  for (const Slot& slot : snapshot) {
    if (slot.entry.symbol != kAnySymbol && slot.entry.symbol != trade.symbol)
      continue;
    try {
      slot.entry.observer->on_trade(trade);
    } catch (...) {
      unsubscribe(slot.id);
    }
  }
}

**How the tests verify you:** the fan-out tests assert every subscriber is
notified in subscription order (a no-op publish leaves everything red); the
unsubscribe test checks removal + unknown-id `false`; the mid-publish
subscription test proves the snapshot semantics (`subscriber_count() == 2` only
*after* the round completes); the filtering test proves per-symbol delivery;
and the throwing-observer test proves eviction while healthy subscribers still
receive events. The concurrency test runs 4 publishers against one subscriber
under `tsan;stress` and asserts zero lost events.