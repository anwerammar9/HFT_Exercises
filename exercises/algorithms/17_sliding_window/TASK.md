# Exercise 40 — Sliding-Window Patterns (Task)

## The problem (in plain words)

A price/volume series is a stream, and window questions are the meat of
market-data math: *what was the widest spread in the last k ticks*, *what is
the short-term max average*, *how short a run reaches a notional target*.
Learn the two window idioms — **fixed-size** (add right, drop left) and
**variable-size** (grow right, shrink left while a constraint holds) — and the
**monotonic queue** trick that makes the fixed-size *maximum* O(n) overall.

## Requirements (what the tests check)

1. `window_maximum(values, k)` — for each window `[i, i+k)`, the maximum,
   O(n) total, monotonic-deque style.
2. `window_sums(values, k)` — `int64` running sum per window; `k == 0` and
   `k > n` both mean empty.
3. `max_average_window(values, k)` — the max of `window_sum / k` as a `double`;
   `k == 0` **throws `std::invalid_argument`**, `k > n` → `0.0`.
4. `min_subarray_len(values, target)` — shortest subarray (contiguous) with
   sum ≥ `target` (`int64`); `0` if none (all values non-negative).
5. `longest_distinct_run(values, k)` — longest contiguous span using **at most
   `k` distinct** values.

## Public API

```cpp
namespace sliding_window {
std::vector<int> window_maximum(const std::vector<int>& values, std::size_t k);
std::vector<std::int64_t> window_sums(const std::vector<int>& values, std::size_t k);
double max_average_window(const std::vector<int>& values, std::size_t k);
std::size_t min_subarray_len(const std::vector<int>& values, std::int64_t target);
std::size_t longest_distinct_run(const std::vector<int>& values, std::size_t k);
}
```

Stub in `src/sliding_window.cpp`, declarations in `include/sliding_window.h`.

## How to think about it (suggested design)

- `window_maximum`: a deque of **indices**. Before pushing `i`, pop the back
  while the tail value is `≤ values[i]` (it can never win again); pop the front
  once it exits the window. Front = current max. Each element enters and leaves
  once → O(n).
- `window_sums`: add the new right value, subtract the value leaving at `i-k`
  once the window is full — O(n), trivial.
- `min_subarray_len`: grow `right` until `sum >= target`, then shrink `left`
  while the sum still qualifies, tracking the shortest span.
- `longest_distinct_run`: track a distinct-count map; when it exceeds `k`,
  shrink `left` until one value is fully evicted.

## Make it harder (optional — not covered by the tests)

- **`peaks_in_window`:** count local maxima/minima inside each fixed window
  (the microstructure "whipsaw" detector).
- **Variable-k max:** for each `right`, the max over the window ending at
  `right` for *every* `k` (monotonic-stack flavor of the same idea).
- **`min_window_containing`:** shortest subarray containing *all* values of a
  given "symbol set" (window + count map, O(n)).
- **Longest window with sum ≤ target:** the dual of `min_subarray_len`.

## Files

- Stub: `src/sliding_window.cpp`
- Tests: `test/test_sliding_window.cpp`
- Reference: `SOLUTION.md`
