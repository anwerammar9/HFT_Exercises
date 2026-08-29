# Exercise 03 — TWAP / VWAP Slicer (Task)

## The problem (in plain words)

A trader places one large parent order. The execution algo breaks it into many
smaller child orders and schedules each at a time: **TWAP** sends equal pieces
evenly spaced across the day, **VWAP** sizes each piece in proportion to the
expected volume of its time bucket. The hard part is *integer rounding*: no
matter what, the child sizes must add up to **exactly** the parent size.

## Requirements (what the tests check)

### TwapSlicer

1. `plan()` splits `total_qty` into `n_slices` pieces, spaced evenly across
   `duration`.
2. `send_at` of slice `i` (0-based) = `duration · (i+1) / n`.
3. Base slice size = `total / n`; the `total % n` remainder is absorbed by the
   **last** slice, so `Σ qty == total` **exactly** (integer arithmetic, no
   floating point).
4. `n == 1` → one slice `{total, 0ms}` (ship the whole parent immediately).
5. `n == 0` or `total <= 0` → empty plan.

### VwapSlicer

1. `plan()` sizes each slice by its normalized `fractions` weight, rounded with
   the **largest-remainder** method so `Σ qty == total` **exactly**.
2. `send_at` of slice `i` = `duration · (cumulative normalized fraction through
   i)` — i.e. the end of that volume bucket.
3. Exactly one fraction → one slice `{total, 0ms}`.
4. Empty, all-zero fractions, or `total <= 0` → empty plan.

> Largest remainder in one sentence: give every slice its floor
> `total · fraction_i`, then hand out the leftover `1`-units one at a time to
> the slices with the biggest fractional remainders until nothing is left.

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

## How to think about it (suggested design)

- TWAP: a simple `for` loop. `send_at` formula given above; compare two
  adjacent slices to be sure no time bucket overlaps or is skipped.
- VWAP: first **normalize** the fractions (sum them, divide each by the total);
  compute each slice's *raw* size as `total · fraction`; then round down with
  `int` truncation, accumulate the fractional parts, and distribute the
  remainder to the largest remainders **in descending order of the leftover**.
  Verify `Σ qty == total` for any inputs you can invent.
- Prefer `std::int64_t` throughout; never round trip through floating-point for
  the TWAP math.

## Make it harder (optional — not covered by the tests)

- **Time-of-day anchoring:** add a `start` instant so `send_at` is an absolute
  time offset rather than duration-relative (real algos schedule against market
  hours).
- **Catch-up rule:** define what happens if the caller misses `send_at` of
  slice *i* — e.g. a `plan(late_now)` that emits the overdue remainder early.
- **Randomize without losing exactness:** a "random slices" variant that still
  satisfies `Σ qty == total` (percent-remainder allocation, not byte tricks).
- **VWAP by traded value:** weight buckets by `forecast_price · forecast_qty`
  instead of volume alone, keeping the sum exact.

## Files

- Stub: `src/twap_vwap_slicer.cpp`
- Tests: `test/test_twap_vwap_slicer.cpp`
- Reference: `SOLUTION.md`