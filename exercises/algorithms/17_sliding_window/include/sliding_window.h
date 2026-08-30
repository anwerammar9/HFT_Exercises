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