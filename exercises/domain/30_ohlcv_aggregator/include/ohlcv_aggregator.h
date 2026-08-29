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