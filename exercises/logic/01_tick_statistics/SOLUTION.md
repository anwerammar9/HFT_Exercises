# Exercise logic/01_tick_statistics (ex01) — Tick Statistics (Reference Solution)

**What you implement:** live per-symbol market-data stats with O(1) online
accumulators for mean/variance/min/max/vwap/last, plus a retained price series
so EWMA can be answered for any per-call alpha.

**Approach**
- `add_tick(price, qty)`: fold into `count_`, `last_`, `min_`, `max_`, `sum_`,
  `sum_sq_` (for variance), `pq_sum_`+`q_sum_` (for vwap), and append to
  `prices_`. All O(1).
- `mean()` = `sum/count`; `variance()` = POPULATION variance via
  `E[x^2] - E[x]^2` (`sum_sq_/n - mean^2`); both 0.0 when empty.
- `vwap()` = `sum(price*qty)/sum(qty)`, 0.0 when no volume.
- `ewma(alpha)`: seeded with the first tick, then `ema = alpha*p_i +
  (1-alpha)*ema` over the whole retained series (per-call alpha ⇒ the series
  must be kept; the streaming alternative is logarithmic buckets).
- All readers are `noexcept` and safely answer on an empty stream.

## Reference API — `include/tick_statistics.h`
#ifndef EXERCISE01_TICK_STATISTICS_H_
#define EXERCISE01_TICK_STATISTICS_H_

#include <cstdint>
#include <vector>

// Live per-symbol market-data statistics.
//
// Contract:
//   - add_tick(price, qty): observe one trade/tick. All stats update in O(1)
//     except the histogram vector, which grows with distinct ticks.
//   - count(): number of ticks observed.
//   - last()/min()/max(): last/high/low price; all are 0.0 when count()==0.
//   - mean(): arithmetic mean of the tick prices (not volume weighted).
//   - variance(): POPULATION variance of the tick prices (divide by n).
//   - vwap(): volume-weighted average price (weight = qty of each tick).
//   - ewma(alpha): exponential moving average of the price series,
//     ema_i = alpha*p_i + (1-alpha)*ema_{i-1}, seeded with the first tick,
//     for an arbitrary alpha supplied per call (0 <= alpha <= 1).
//
// Required design decisions (pick & document):
//   - keep O(1) accumulators for mean/variance (sum, sum of squares),
//     min/max, vwap (sum price*qty, sum qty), last;
//   - keep the price series (a std::vector) so ewma(alpha) can be answered
//     for any alpha — note the streaming alternative (logarithmic buckets)
//     if you want to avoid unbounded growth.
//
// Single-threaded. This is the kind of state a market-data feed handler keeps
// per symbol between request snapshots.

class TickStatistics {
 public:
  void add_tick(double price, std::uint64_t qty);

  std::uint64_t count() const noexcept { return count_; }
  double last() const noexcept { return last_; }
  double min() const noexcept { return count_ ? min_ : 0.0; }
  double max() const noexcept { return count_ ? max_ : 0.0; }
  double mean() const noexcept;
  double variance() const noexcept;
  double vwap() const noexcept;
  double ewma(double alpha) const noexcept;

 private:
  std::uint64_t count_ = 0;
  double last_ = 0.0;
  double min_ = 0.0;
  double max_ = 0.0;
  double sum_ = 0.0;
  double sum_sq_ = 0.0;
  double pq_sum_ = 0.0;   // sum(price * qty): for vwap
  std::uint64_t q_sum_ = 0;  // sum(qty): for vwap
  std::vector<double> prices_;  // for ewma(alpha); see design note
};

#endif  // EXERCISE01_TICK_STATISTICS_H_
## Reference implementation — `src/tick_statistics.cpp`
#include "tick_statistics.h"

#include <algorithm>

void TickStatistics::add_tick(double price, std::uint64_t qty) {
  if (count_ == 0) {
    min_ = max_ = price;
  } else {
    min_ = std::min(min_, price);
    max_ = std::max(max_, price);
  }
  ++count_;
  last_ = price;
  sum_ += price;
  sum_sq_ += price * price;
  pq_sum_ += price * static_cast<double>(qty);
  q_sum_ += qty;
  prices_.push_back(price);
}

double TickStatistics::mean() const noexcept {
  return count_ == 0 ? 0.0 : sum_ / static_cast<double>(count_);
}

double TickStatistics::variance() const noexcept {
  if (count_ == 0) return 0.0;
  const double mu = mean();
  return sum_sq_ / static_cast<double>(count_) - mu * mu;  // population variance
}

double TickStatistics::vwap() const noexcept {
  return q_sum_ == 0 ? 0.0 : pq_sum_ / static_cast<double>(q_sum_);
}

double TickStatistics::ewma(double alpha) const noexcept {
  if (prices_.empty()) return 0.0;
  double ema = prices_.front();
  for (std::size_t i = 1; i < prices_.size(); ++i) {
    ema = alpha * prices_[i] + (1.0 - alpha) * ema;
  }
  return ema;
}