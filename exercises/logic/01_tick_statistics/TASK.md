# Exercise 01 — Tick Statistics (Task)

## The problem (in plain words)

A market-data feed handler has to answer questions like *"what is the average
price hit so far?"* or *"what is the VWAP since the session started?"* without
ever stopping to scan old ticks. Every incoming trade must fold into a set of
running statistics in **O(1)** — plus the raw price series is kept so an
exponential moving average can be computed for *any* smoothing factor `alpha`
after the fact.

## Requirements (what the tests check)

1. `add_tick(price, qty)` records one trade and updates **all** statistics in
   constant time — you may never loop over past ticks inside `add_tick`.
2. `count()` = number of ticks seen so far.
3. `last()` / `min()` / `max()` = the most recent / lowest / highest price
   observed. All three return **0.0** while `count() == 0`.
4. `mean()` = simple arithmetic mean of the tick **prices** (ignore `qty`).
   `variance()` = **population** variance, i.e. the sum of squared deviations
   from the mean **divided by n** (not n−1).
5. `vwap()` = Σ(price·qty) / Σ(qty).
6. `ewma(alpha)` = exponential moving average of the price series:
   `ema_i = alpha·p_i + (1−alpha)·ema_{i-1}`, seeded with the first tick.
   It must give a correct answer for any `alpha` in [0, 1], passed fresh on
   each call.

### Why these exact formulas
Everything is derived from a handful of accumulators. `mean` needs `sum`, the
variance needs `sum` **and** `sum of squares` (`var = Σx²/n − (Σx/n)²`); VWAP
needs `Σ(price·qty)` and `Σqty`. If you keep exactly those five or six numbers
plus `last/min/max`, every query is O(1).

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

## How to think about it (suggested design)

- Plain member variables: `sum_`, `sum_sq_` (for mean/variance), `pq_sum_`,
  `q_sum_` (for VWAP), `last_/min_/max_`, and a `std::vector<double>` holding
  every price so `ewma()` can be answered for any `alpha`.
- `add_tick` just updates the counters and pushes the price; every getter
  reduces to its accumulators. Nothing fancy — no templates, no type tricks.
- Single-threaded by design (one feed handler owns one symbol's stats).

## Make it harder (optional — not covered by the tests)

- **Multi-symbol:** wrap this class in a `SymbolStatsTable` that keeps one
  `TickStatistics` per symbol in a `std::map<std::string, TickStatistics>`.
- **Impervious variance:** implement **Welford's online algorithm** instead of
  `Σx²/n − (Σx/n)²` so huge prices don't cancel catastrophically; assert both
  give the same answer on small inputs.
- **Live percentiles:** keep a coarse sorted histogram of price buckets so you
  can answer approximate `p50`/`p99` on demand without storing every price.
- **Stop the unbounded vector:** research *logarithmic buckets* — an O(log n)
  streaming summary that lets `ewma` drop the per-tick storage.

## Files

- Stub: `src/tick_statistics.cpp`
- Tests: `test/test_tick_statistics.cpp`
- Reference: `SOLUTION.md`