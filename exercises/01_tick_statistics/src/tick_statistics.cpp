#include "tick_statistics.h"

// TODO(anwer): implement the O(1) accumulators (see SOLUTION.md).
//
// Suggested shape:
//   add_tick(price, qty):  update last_/min_/max_, sum_, sum_sq_, pq_sum_,
//                          q_sum_, count++, push price onto prices_.
//   mean()      = count_ ? sum_ / (double)count_ : 0.0;
//   variance()  = count_ ? sum_sq_/n - mean()*mean() : 0.0;   // population
//   vwap()      = q_sum_ ? pq_sum_ / (double)q_sum_ : 0.0;
//   ewma(alpha) = seeded first price, run the recurrence over prices_.

void TickStatistics::add_tick(double /*price*/, std::uint64_t /*qty*/) {}

double TickStatistics::mean() const noexcept { return 0.0; }

double TickStatistics::variance() const noexcept { return 0.0; }

double TickStatistics::vwap() const noexcept { return 0.0; }

double TickStatistics::ewma(double /*alpha*/) const noexcept { return 0.0; }