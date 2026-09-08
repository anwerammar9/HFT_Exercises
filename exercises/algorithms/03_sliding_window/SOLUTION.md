# Exercise algorithms/03_sliding_window (ex40) — Sliding-Window Patterns (Reference Solution)

**What you implement:** the three sliding-window movement patterns over a
price/volume series — FIXED-window (monotonic-deque maximum + running sums +
max average), VARIABLE-window grow-right/shrink-left (shortest subarray reaching
a notional target), and VARIABLE-window with a distinct-count constraint
(longest span using ≤ k distinct symbols).

**Approach**
- **window_maximum:** a monotonic deque of *indices*. Before pushing index `i`,
  pop back while the tail value is ≤ `values[i]` (it can never win again); pop
  the front once it leaves the window (`q.front() + k <= i`). The front is the
  window max. O(n) overall, O(k) memory.
- **window_sums:** a rolling 64-bit accumulator — add the new right value,
  subtract the leaving left value (index `i-k`) once `i >= k`; emit from
  `i+1 >= k`. `std::int64_t` keeps a long window of large ints overflow-free.
- **max_average_window:** delegate to `window_sums`, return
  `max(sum)/k` as a double; `k == 0` throws `std::invalid_argument`, `k > n`
  returns `0.0`.
- **min_subarray_len:** the classic variable window over non-negative values:
  grow `right` until the running sum reaches `target`, then shrink `left` while
  it still does, tracking the shortest span; `best > n` → `0` (no window hit).
- **longest_distinct_run:** grow `right`, count distinct values in a hash map,
  and when the count exceeds `k` shrink `left` until one distinct value is
  fully evicted; `best` is the longest window seen.

## Reference API — `include/sliding_window.h`
#ifndef EXERCISE40_SLIDING_WINDOW_H_
#define EXERCISE40_SLIDING_WINDOW_H_

#include <cstddef>
#include <cstdint>
#include <vector>

// Classic sliding-window patterns over a price/volume series. The window is
// "slid" by adding one element on the right and dropping one on the left, so a
// whole overlapping family of subarrays is evaluated in one pass.
//
// Contract:
//   - window_maximum / window_sums / max_average_window use a FIXED window of
//     size k; the result has length n-k+1 (empty when k == 0 or k > n).
//     window_maximum MUST use a monotonic deque (O(n) overall, O(n) extra
//     memory in the worst case); window_sums is a running sum.
//   - min_subarray_len uses a VARIABLE window that grows right and shrinks
//     left; input values are non-negative and returns the shortest length
//     whose sum >= target, or 0 when no window reaches it.
//   - longest_distinct_run returns the longest contiguous span containing at
//     most k DISTINCT values (k == 0 -> 0).
//   - max_average_window throws std::invalid_argument for k == 0 and returns
//     0.0 when k > n.
//
// TODO(anwer): implement the window loops (see SOLUTION.md). The stubs return
// empty/0/0.0 so every window test runs RED without hanging or crashing.

namespace sliding_window {

// Max of every length-k window, left to right (monotonic-deque pattern).
std::vector<int> window_maximum(const std::vector<int>& values, std::size_t k);

// Sum (64-bit so a long window of large ints cannot overflow) of every
// length-k window, left to right. Running-sum pattern.
std::vector<std::int64_t> window_sums(const std::vector<int>& values,
                                      std::size_t k);

// Largest average of any length-k window (throwing on k == 0).
double max_average_window(const std::vector<int>& values, std::size_t k);

// Shortest contiguous subarray whose sum >= target (non-negative values);
// 0 when none exists. Variable window: grow right, shrink left.
std::size_t min_subarray_len(const std::vector<int>& values,
                             std::int64_t target);

// Longest contiguous span with at most k distinct values.
std::size_t longest_distinct_run(const std::vector<int>& values,
                                 std::size_t k);

}  // namespace sliding_window

#endif  // EXERCISE40_SLIDING_WINDOW_H_
## Reference implementation — `src/sliding_window.cpp`
#include "sliding_window.h"

#include <algorithm>
#include <deque>
#include <stdexcept>
#include <unordered_map>

namespace sliding_window {

std::vector<int> window_maximum(const std::vector<int>& values,
                                std::size_t k) {
  const std::size_t n = values.size();
  if (k == 0 || k > n) return {};
  std::vector<int> out;
  out.reserve(n - k + 1);
  std::deque<std::size_t> q;  // indices, values non-increasing front->back
  for (std::size_t i = 0; i < n; ++i) {
    while (!q.empty() && values[q.back()] <= values[i]) q.pop_back();
    q.push_back(i);
    if (q.front() + k <= i) q.pop_front();  // fell out of the window
    if (i + 1 >= k) out.push_back(values[q.front()]);
  }
  return out;
}

std::vector<std::int64_t> window_sums(const std::vector<int>& values,
                                      std::size_t k) {
  const std::size_t n = values.size();
  if (k == 0 || k > n) return {};
  std::vector<std::int64_t> out;
  out.reserve(n - k + 1);
  std::int64_t run = 0;
  for (std::size_t i = 0; i < n; ++i) {
    run += values[i];
    if (i >= k) run -= values[i - k];
    if (i + 1 >= k) out.push_back(run);
  }
  return out;
}

double max_average_window(const std::vector<int>& values, std::size_t k) {
  if (k == 0) throw std::invalid_argument("max_average_window: k == 0");
  const auto sums = window_sums(values, k);
  if (sums.empty()) return 0.0;
  const auto best = *std::max_element(sums.begin(), sums.end());
  return static_cast<double>(best) / static_cast<double>(k);
}

std::size_t min_subarray_len(const std::vector<int>& values,
                             std::int64_t target) {
  std::size_t left = 0;
  std::int64_t run = 0;
  std::size_t best = values.size() + 1;
  for (std::size_t right = 0; right < values.size(); ++right) {
    run += values[right];
    while (run >= target) {
      best = std::min(best, right - left + 1);
      run -= values[left++];
      if (left > right) break;
    }
  }
  return best > values.size() ? 0 : best;
}

std::size_t longest_distinct_run(const std::vector<int>& values,
                                 std::size_t k) {
  if (k == 0) return 0;
  std::unordered_map<int, std::size_t> counts;
  std::size_t left = 0;
  std::size_t distinct = 0;
  std::size_t best = 0;
  for (std::size_t right = 0; right < values.size(); ++right) {
    if (counts[values[right]]++ == 0) ++distinct;
    while (distinct > k) {
      if (--counts[values[left]] == 0) --distinct;
      ++left;
    }
    best = std::max(best, right - left + 1);
  }
  return best;
}

}  // namespace sliding_window