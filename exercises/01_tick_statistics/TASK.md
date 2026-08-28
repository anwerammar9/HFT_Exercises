# Exercise 01 — Tick Statistics (Task)

## Problem
Maintain live per-symbol market-data statistics: each incoming tick updates
O(1) accumulators (mean/variance/min/max/vwap/last) **plus** a retained price
series so an exponential moving average can be computed for any `alpha` at any
time.

## Requirements (what the tests check)
1. `add_tick(price, qty)` folds a tick into every statistic; all readers are
   `noexcept`.
2. `count()` = number of ticks seen.
3. `last()`/`min()`/`max()` = last/high/low price; **0.0 when `count() == 0`**.
4. `mean()` = arithmetic mean of prices (not volume-weighted); `variance()` =
   **population** variance (divide by n).
5. `vwap()` = Σ(price·qty) / Σqty.
6. `ewma(alpha)` = `ema_i = alpha·p_i + (1−alpha)·ema_{i-1}`, seeded with the
   first tick, valid for any `alpha` in [0,1] passed per call.

## Public API
```cpp
class TickStatistics {
  void add_tick(double price, std::uint64_t qty);
  std::uint64_t count() const noexcept;
  double last() const noexcept;
  double min() const noexcept;
  double max() const noexcept;
  double mean() const noexcept;
  double variance() const noexcept;
  double vwap() const noexcept;
  double ewma(double alpha) const noexcept;
};
```

## Design notes
Keep `sum`/`sum_sq` for variance, `sum(price·qty)`/`sum(qty)` for VWAP, and a
`std::vector<double>` of prices for EWMA. Single-threaded.

## Files
- Stub: `src/tick_statistics.cpp`
- Tests: `test/test_tick_statistics.cpp`
- Reference: `SOLUTION.md`