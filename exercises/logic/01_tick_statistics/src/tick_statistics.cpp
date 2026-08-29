#include "tick_statistics.h"

// TODO(anwer): implement the real accumulators (see SOLUTION.md).
// Stub: add_tick is a no-op and every statistic answers the empty-stream
// value (0.0), so the tests fail RED.

void TickStatistics::add_tick(double /*price*/, std::uint64_t /*qty*/) {}

double TickStatistics::mean() const noexcept { return 0.0; }

double TickStatistics::variance() const noexcept { return 0.0; }

double TickStatistics::vwap() const noexcept { return 0.0; }

double TickStatistics::ewma(double /*alpha*/) const noexcept { return 0.0; }