# Exercise domain/07_ohlcv_aggregator (ex30) — OHLCV Candle Aggregator (Reference Solution)

**What you implement:** a single-pass time-bucket state machine that folds an
as-if-ordered trade stream into gap-free OHLCV candles.

**Approach**
- Four members do all the work: `current_bucket_` (the open bucket index,
  `-1` = none), `current_`, `has_trade_`, `completed_`.
- `add_trade(ts, price, qty)`: `b = ts.count() / bucket_ms_.count()`
  - open a bucket when none is open: push empty candles for buckets `0..b-1`
    (leading gap), set `current_bucket_ = b`, set `open_time = b * W`;
  - `b < current_bucket_` → return (stale); the candle is already finalized;
  - `b > current_bucket_` → push `current_` to `completed_`, push empty candles
    for `current_bucket_+1 .. b-1` (interior gap), open bucket `b`;
  - fold: `!has_trade_` → `open=high=low=close=price`; else
    `high=max, low=min, close=price`; `volume += qty`.
- `roll()`: append `current_` (even empty) to `completed_`, return a copy,
  reset to an empty candle at the same bucket's `open_time`. If no bucket was
  ever opened it's a no-op returning an empty candle.
- Everything is integer bucket arithmetic + a monotone fold; no clock needed —
  determinism is the feature.

## Reference API — `include/ohlcv_aggregator.h`

```cpp
#ifndef EXERCISE30_OHLCV_AGGREGATOR_H_
#define EXERCISE30_OHLCV_AGGREGATOR_H_

#include <chrono>
#include <cstdint>
#include <vector>

// Time-bucketed OHLCV candle aggregator.
//
// Turns a stream of (timestamp, price, quantity) trades into fixed-duration
// candles. EVERY bucket is produced — not just the ones that saw a trade —
// so a chart plotting `completed()` has no time gaps.
//
// Bucketing:
//   - A trade at `ts` belongs to bucket `floor(ts / bucket_ms)`.
//   - The timeline ALWAYS starts at bucket 0: a first trade that lands in
//     bucket B emits empty candles for buckets 0..B-1 into completed() first.
//   - Every candle's `open_time` is the millisecond when its bucket started
//     (bucket_index * bucket_ms).
//
// Contract:
//   - Trades must arrive in NON-DECREASING timestamp order (like a real
//     market-data feed). A trade in an EARLIER bucket than the current one is
//     IGNORED — its candle is already finalized and cannot be amended.
//   - add_trade(ts, price, qty):
//       older bucket -> ignored.
//       current or later bucket -> folded into the current candle.
//       later bucket -> the current candle is finalized to completed(), empty
//       candles are appended for every skipped intermediate bucket, then the
//       trade opens the new bucket's candle.
//   - current(): the in-progress candle. A candle that has seen no trade is
//     empty (all fields 0, `open_time` set) — a normal state between roll()
//     and the next trade.
//   - roll(): finalizes the current candle (even if it is empty) into
//     completed(), returns a copy of it, and resets the accumulator. The next
//     add_trade() starts a BRAND-NEW candle at the current bucket's start
//     (new open_time only when a later bucket arrives).
//   - bucket_ms() must be positive (non-positive throws std::invalid_argument).
//
// TODO(anwer): implement in src/ohlcv_aggregator.cpp.
//   - State: current_bucket_ (== -1 while no bucket is open), current_,
//     has_trade_, completed_.
//   - On add_trade compute b = ts / bucket_ms:
//       * current_bucket_ == -1 : emit leading empties 0..b-1, open bucket b.
//       * b <  current_bucket_  : ignore (stale trade).
//       * b >  current_bucket_  : finalize current_, emit empties
//         current_bucket_+1 .. b-1, open bucket b.
//       * then fold the trade (open/high/low/close/volume).
//   - fold: if !has_trade_ -> open = high = low = close = price;
//           else high = max, low = min, close = price; volume += qty always.

class OhlcvAggregator {
 public:
  struct Candle {
    std::chrono::milliseconds open_time{0};
    double open = 0.;
    double high = 0.;
    double low = 0.;
    double close = 0.;
    std::uint64_t volume = 0;  // summed quantity; never fractional
  };

  explicit OhlcvAggregator(std::chrono::milliseconds bucket_ms);

  OhlcvAggregator(const OhlcvAggregator&) = delete;
  OhlcvAggregator& operator=(const OhlcvAggregator&) = delete;

  void add_trade(std::chrono::milliseconds ts, double price, std::uint64_t qty);

  // In-progress candle; a snapshot of internal state (do not retain).
  const Candle& current() const noexcept;
  // Every finalized candle, oldest first.
  const std::vector<Candle>& completed() const noexcept;
  std::chrono::milliseconds bucket_ms() const noexcept;

  // Finalize + return the current candle, then reset for a fresh start.
  Candle roll();

 private:
  std::chrono::milliseconds bucket_ms_;
  std::int64_t current_bucket_ = -1;  // -1: no bucket open yet
  Candle current_;
  std::vector<Candle> completed_;
  bool has_trade_ = false;
};

#endif  // EXERCISE30_OHLCV_AGGREGATOR_H_
```

## Reference implementation — `src/ohlcv_aggregator.cpp`

```cpp
#include "ohlcv_aggregator.h"

#include <algorithm>
#include <stdexcept>

namespace {

void AppendEmptyCandle(std::vector<OhlcvAggregator::Candle>* out,
                       std::chrono::milliseconds open_time) {
  OhlcvAggregator::Candle empty;
  empty.open_time = open_time;
  out->push_back(empty);
}

}  // namespace

OhlcvAggregator::OhlcvAggregator(std::chrono::milliseconds bucket_ms)
    : bucket_ms_(bucket_ms) {
  if (bucket_ms <= std::chrono::milliseconds(0)) {
    throw std::invalid_argument("bucket duration must be positive");
  }
}

void OhlcvAggregator::add_trade(std::chrono::milliseconds ts, double price,
                                std::uint64_t qty) {
  const std::int64_t b = ts.count() / bucket_ms_.count();

  if (current_bucket_ == -1) {
    // First trade: open the timeline at bucket 0, emit leading empties up to b.
    for (std::int64_t k = 0; k < b; ++k) {
      AppendEmptyCandle(&completed_, std::chrono::milliseconds(k * bucket_ms_.count()));
    }
    current_bucket_ = b;
    current_.open_time = std::chrono::milliseconds(b * bucket_ms_.count());
  } else if (b < current_bucket_) {
    return;  // stale trade: its candle is already finalized
  } else if (b > current_bucket_) {
    // Finalize the previous candle, emit empty candles for skipped buckets.
    completed_.push_back(current_);
    for (std::int64_t k = current_bucket_ + 1; k < b; ++k) {
      AppendEmptyCandle(&completed_, std::chrono::milliseconds(k * bucket_ms_.count()));
    }
    current_bucket_ = b;
    current_ = Candle{};
    current_.open_time = std::chrono::milliseconds(b * bucket_ms_.count());
    has_trade_ = false;
  }

  if (!has_trade_) {
    current_.open = current_.high = current_.low = current_.close = price;
    has_trade_ = true;
  } else {
    current_.high = std::max(current_.high, price);
    current_.low = std::min(current_.low, price);
    current_.close = price;
  }
  current_.volume += qty;
}

const OhlcvAggregator::Candle& OhlcvAggregator::current() const noexcept {
  return current_;
}

const std::vector<OhlcvAggregator::Candle>& OhlcvAggregator::completed()
    const noexcept {
  return completed_;
}

std::chrono::milliseconds OhlcvAggregator::bucket_ms() const noexcept {
  return bucket_ms_;
}

OhlcvAggregator::Candle OhlcvAggregator::roll() {
  Candle finished = current_;
  if (current_bucket_ != -1) {
    completed_.push_back(current_);
  }
  current_ = Candle{};
  if (current_bucket_ != -1) {
    current_.open_time = std::chrono::milliseconds(current_bucket_ * bucket_ms_.count());
  }
  has_trade_ = false;
  return finished;
}
```

**How the tests verify you:** the fold fields (`TradesWithinSameBucketFold`), the
gap-emission state machine (`LaterBucketFinalizesPreviousAndEmitsGapEmpties`,
`FirstTradeInLaterBucketEmitsLeadingEmpties`), stale-trade rejection
(`StaleOlderBucketTradeIsIgnored`), and the reset semantics of `roll()`.
The stub throws `logic_error` from mutators so those tests are instantly red.