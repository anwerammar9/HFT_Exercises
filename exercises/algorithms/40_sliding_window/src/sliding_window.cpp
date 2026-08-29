#include "sliding_window.h"

// TODO(anwer): implement the window loops (see SOLUTION.md).
//
// Suggested shape:
//   - window_maximum: monotonic deque of indices (values non-increasing front
//     to back inside the window).
//   - window_sums: running int64 sum, subtract the leaving left element.
//   - max_average_window: max window_sum / k.
//   - min_subarray_len: grow right until sum >= target, then shrink left,
//     tracking the shortest length.
//   - longest_distinct_run: grow right counting distinct values, shrink left
//     while more than k are present.
//
// Stub: every window is empty / zero, so the sliding-window tests run RED
// without hanging or crashing.

namespace sliding_window {

std::vector<int> window_maximum(const std::vector<int>& /*values*/,
                                std::size_t /*k*/) {
  return {};
}

std::vector<std::int64_t> window_sums(const std::vector<int>& /*values*/,
                                      std::size_t /*k*/) {
  return {};
}

double max_average_window(const std::vector<int>& /*values*/, std::size_t /*k*/) {
  return 0.0;
}

std::size_t min_subarray_len(const std::vector<int>& /*values*/,
                             std::int64_t /*target*/) {
  return 0;
}

std::size_t longest_distinct_run(const std::vector<int>& /*values*/,
                                 std::size_t /*k*/) {
  return 0;
}

}  // namespace sliding_window