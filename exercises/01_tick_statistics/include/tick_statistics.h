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