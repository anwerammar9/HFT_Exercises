# Exercise 03 — TWAP / VWAP Slicer (Task)

## Problem
Slice a parent order into child orders — TWAP evenly across time, VWAP in
proportion to forecast volume per bucket — where integer rounding guarantees
the slices sum **exactly** to the parent size.

## Requirements (what the tests check)

### TwapSlicer
1. `plan()` splits `total_qty` across `n_slices` evenly in time.
2. `send_at` of slice *i* (0-based) = `duration · (i+1) / n`.
3. Base = `total/n`; remainder `total % n` absorbed by the **last** slice, so
   `Σ qty == total` exactly.
4. `n == 1` → one slice `{total, 0ms}`.
5. `n == 0` or `total <= 0` → empty plan.

### VwapSlicer
1. `plan()` weights slice size by `fractions` (normalized; need not sum to 1),
   rounded via **largest remainder** so `Σ qty == total` exactly.
2. `send_at` of slice *i* = `duration · cumulative normalized fraction through i`
   (the end of that volume bucket).
3. Exactly one fraction → one slice `{total, 0ms}`.
4. Empty / all-zero fractions or `total <= 0` → empty plan.

## Public API
```cpp
using Qty = std::int64_t;
struct Slice { Qty qty; std::chrono::milliseconds send_at; };

class TwapSlicer {
  TwapSlicer(Qty total_qty, std::chrono::milliseconds duration, std::size_t n_slices);
  std::vector<Slice> plan() const;
};

class VwapSlicer {
  VwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
             std::vector<double> fractions);
  std::vector<Slice> plan() const;
};
```

## Files
- Stub: `src/twap_vwap_slicer.cpp`
- Tests: `test/test_twap_vwap_slicer.cpp`
- Reference: `SOLUTION.md`