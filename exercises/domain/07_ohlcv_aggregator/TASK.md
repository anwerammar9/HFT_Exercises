# Exercise domain/07_ohlcv_aggregator (ex30) — OHLCV Candle Aggregator (Task)

## The problem (in plain words)

Real market data arrives as a firehose of individual trades
`(timestamp, price, qty)`. Nobody plots trades. This exercise compresses that
stream into **time-bucketed OHLCV candles**: for each fixed-duration bucket
(e.g. 1 second) you track the bucket's **O**pen (first trade price), **H**igh,
**L**ow, **C**lose (last trade price) and **V**olume (sum of quantities). The
twist that makes it a real component: every bucket shows up, empty or not — a
chart drawn from `completed()` must have no gaps.

## Requirements (what the tests check)

1. `OhlcvAggregator(bucket_ms)` with `bucket_ms <= 0` throws
   `std::invalid_argument`.
2. A trade at `ts` folds into bucket `floor(ts / bucket_ms)`; the candle's
   `open_time` is that bucket's start time.
3. Trades within one bucket fold together: `open` stays the first trade's
   price, `high`/`low` track extremes, `close` is the last trade's price,
   `volume` sums quantities.
4. A trade in a **later** bucket finalizes the previous candle into
   `completed()`, **emits an empty candle for every skipped bucket** in
   between, then opens the new bucket's candle.
5. A trade in an **earlier** bucket than the current one (late/stale delivery)
   is **ignored** — the finalized candle can't be amended.
6. The first trade may land anywhere: buckets `0..B-1` leading up to it are
   emitted as empties too (the timeline always starts at time 0).
7. `roll()` finalizes and returns the current candle (even if empty), appends
   it to `completed()`, and resets the accumulator; the next trade starts a
   brand-new candle.

## Public API

```cpp
class OhlcvAggregator {
 public:
  struct Candle {
    std::chrono::milliseconds open_time{0};
    double open = 0., high = 0., low = 0., close = 0.;
    std::uint64_t volume = 0;
  };

  explicit OhlcvAggregator(std::chrono::milliseconds bucket_ms);

  void add_trade(std::chrono::milliseconds ts, double price, std::uint64_t qty);
  const Candle& current() const noexcept;
  const std::vector<Candle>& completed() const noexcept;
  std::chrono::milliseconds bucket_ms() const noexcept;
  Candle roll();
 private:
  std::chrono::milliseconds bucket_ms_;
  std::int64_t current_bucket_ = -1;
  Candle current_;
  std::vector<Candle> completed_;
  bool has_trade_ = false;
};
```

## How to think about it (suggested design)

- The whole exercise is one small state machine driven by
  `b = ts.count() / bucket_ms_.count()`:
  1. no bucket open yet (`current_bucket_ == -1`) → emit leading empty candles
     `0..b-1`, then open bucket `b`;
  2. `b < current_bucket_` → ignore (stale trade);
  3. `b > current_bucket_` → push `current_` to `completed_`, emit empty
     candles `current_bucket_+1 .. b-1`, open bucket `b`;
  4. fold the trade into `current_`.
- Fold rule: `has_trade_` false → `open = high = low = close = price`;
  otherwise `high = max(...)`, `low = min(...)`, `close = price`; `volume += qty`
  always.
- `open_time` always equals `bucket_index * bucket_ms` — never the trade's own
  timestamp.
- An empty candle has all price fields `0`. `roll()` returns a *copy*; `current()`
  is a snapshot of live state.
- Single-threaded, time-ordered input — no clock, no timers; the caller is the
  feed. That keeps it deterministic and matchable against a naive reference.

## Make it harder (optional — not covered by the tests)

- **Partial candle hooks:** a `variant`/`callback` notification the moment a
  bucket finalizes (so a publisher can push a completed candle without waiting
  to be polled), wired to a `std::function<void(const Candle&)>` batch emitter.
- **Replacement / correction messages:** `modify_trade(ts, price, qty)` that
  adjusts a bucket if it's still open (intraday price-fix feeds), with the
  `high`/`low` recompute.
- **Bucketing from a live clock:** a `flush(now)` that internally rolls any
  bucket older than `now` (so wall-time-driven consumers get tick-aligned
  candles), built on Exercise 10's wheel.
- **Doubles → fixed point:** re-derive OHLCV with `int64_t` scaled prices
  (`price * 10_000`) to eliminate `double` rounding in `high`/`low`.
- **Differential test:** generate random trade streams ± out-of-order stales and
  compare your output against a `std::map<epoch_ms, Candle>` reference model, so
  the state machine is proven over thousands of scenarios.

## Files

- Stub: `src/ohlcv_aggregator.cpp`
- Tests: `test/test_ohlcv_aggregator.cpp`
- Reference: `SOLUTION.md`