#include "ohlcv_aggregator.h"

#include <stdexcept>

// TODO(anwer): implement the aggregator (see SOLUTION.md).
//
//   - bucket_ms() <= 0 -> throw std::invalid_argument.
//   - On add_trade(ts, price, qty): b = ts.count() / bucket_ms_.count();
//     * current_bucket_ == -1 : emit empty candles for buckets 0..b-1 into
//       completed_, then open bucket b (set open_time).
//     * b < current_bucket_    : return (stale trade, ignore).
//     * b > current_bucket_    : push current_ to completed_, emit empty
//       candles for buckets current_bucket_+1 .. b-1, then open bucket b.
//     * fold: !has_trade_ -> open=high=low=close=price; else update
//       high=max, low=min, close=price; volume += qty.
//   - roll(): push current_ to completed_ (unless no bucket is open), return
//     a copy, reset current_ + has_trade_ (open_time = bucket b start).

OhlcvAggregator::OhlcvAggregator(std::chrono::milliseconds bucket_ms)
    : bucket_ms_(bucket_ms) {
  if (bucket_ms <= std::chrono::milliseconds(0)) {
    throw std::invalid_argument("bucket duration must be positive");
  }
}

void OhlcvAggregator::add_trade(std::chrono::milliseconds /*ts*/,
                                double /*price*/, std::uint64_t /*qty*/) {
  throw std::logic_error("not implemented");
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
  throw std::logic_error("not implemented");
}