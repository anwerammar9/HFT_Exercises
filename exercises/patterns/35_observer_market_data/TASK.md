# Exercise 35 — Observer (Market-Data Fan-out) (Task)

## The problem (in plain words)

A market-data feed pushes quotes and trades to whoever is interested — a
monitor display, the risk desk, a strategy — and none of them should know about
each other. The **Observer** pattern decouples the publisher from its
subscribers. The production refinements this exercise adds: subscriptions can
be **symbol-filtered**, fan-out is **synchronous and ordered** by subscription,
the fan-out runs on a **consistent snapshot** (so a callback that subscribes
doesn't disturb the round in progress), and the whole thing is
**thread-safe** without ever calling user code under a lock.

## Requirements (what the tests check)

1. `subscribe(observer, symbol)` registers an observer and returns a unique
   1-based `ObserverId`. With `symbol == kAnySymbol` the observer hears
   everything; otherwise only events for that symbol.
2. `unsubscribe(id)` removes a subscriber and returns `false` for an unknown /
   already-removed id, or for the invalid id `0`. `subscriber_count()` reports
   the live count.
3. `publish_quote(q)` / `publish_trade(t)` synchronously call
   `on_quote`/`on_trade` on every matching subscriber **in subscription order**.
4. Events route by type: a quote publishes `on_quote`, a trade publishes
   `on_trade`.
5. A subscription made *during* a publish is **not** notified by that same
   publish — it is picked up by the next one (snapshot semantics).
6. A throwing observer is **dropped** by the feed: the remaining subscribers
   still receive the event and `subscriber_count()` shrinks.
7. **Concurrency:** many publisher threads fanning out to one subscriber must
   deliver every event (no lost events under `tsan;stress`).

## Public API

```cpp
using ObserverId = std::uint64_t;
using SymbolId = std::uint64_t;
using PriceMicros = std::int64_t;
inline constexpr SymbolId kAnySymbol = 0;

struct Quote { SymbolId symbol; PriceMicros bid; std::int64_t bid_size;
               PriceMicros ask; std::int64_t ask_size; std::uint64_t seq; };
struct Trade { SymbolId symbol; PriceMicros price; std::int64_t qty;
               std::uint64_t seq; };

class MarketDataObserver {
  virtual void on_quote(const Quote&) = 0;
  virtual void on_trade(const Trade&) = 0;
};

class MarketDataFeed {
  ObserverId subscribe(std::shared_ptr<MarketDataObserver> observer,
                       SymbolId symbol = kAnySymbol);
  bool unsubscribe(ObserverId id);
  std::size_t subscriber_count() const;
  void publish_quote(const Quote& quote);
  void publish_trade(const Trade& trade);
};
```

## How to think about it (suggested design)

- The feed keeps a `std::vector<Slot>` where `Slot = {id, {observer, symbol}}`
  guarded by one mutex; ids come from a monotonically increasing counter.
- The publish path is the design's core: **lock → copy the vector → unlock →
  run callbacks**. Running callbacks outside the lock is what lets a subscriber
  subscribe/unsubscribe from inside its own callback without deadlocking or
  corrupting the round.
- Filtering happens by comparing the slot's `symbol` (or `kAnySymbol`) with the
  event's symbol — before invoking the callback.
- Wrap each callback in try/catch; on throw, `unsubscribe(slot.id)` so a single
  bad subscriber is evicted and can't be re-invoked next round.
- The shared_ptr ownership is what makes subscriber lifetime safe: the feed
  holds a reference, and removing a slot drops the last reference the moment
  it is safe to destroy the observer.

## Make it harder (optional — not covered by the tests)

- **Backpressure:** a slow subscriber (one that spends > X µs in a callback)
  is moved to an async queue or dropped entirely.
- **Priority subscribers:** a `subscribe(observer, symbol, priority)` that
  changes fan-out order (e.g. risk before display).
- **Fast-path no-filter fan-out:** a separate array of symbol-unfiltered
  observers so publishing a "everything" feed skips the per-slot comparison.
- **Reader-writer feed:** snapshot via a copy-on-write `shared_ptr` to the
  subscriber vector so concurrent publishers don't contend on one mutex.

## Files

- Stub: `src/observer_market_data.cpp`
- Tests: `test/test_observer_market_data.cpp`
- Reference: `SOLUTION.md`