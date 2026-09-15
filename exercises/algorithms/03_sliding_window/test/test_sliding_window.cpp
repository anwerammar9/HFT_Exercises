#include <gtest/gtest.h>

#include "sliding_window.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

using sliding_window::longest_distinct_run;
using sliding_window::max_average_window;
using sliding_window::min_subarray_len;
using sliding_window::window_maximum;
using sliding_window::window_sums;

}  // namespace

TEST(SlidingWindowTest, WindowMaximumClassic) {
  const auto out =
      window_maximum({1, 3, -1, -3, 5, 3, 6, 7}, 3);
  EXPECT_EQ(out, (std::vector<int>{3, 3, 5, 5, 6, 7}));
}

TEST(SlidingWindowTest, WindowMaximumEdgeWindows) {
  EXPECT_EQ(window_maximum({4, 1, 2}, 1), (std::vector<int>{4, 1, 2}));
  EXPECT_EQ(window_maximum({4, 1, 2}, 3), (std::vector<int>{4}));
  EXPECT_EQ(window_maximum({4, 1, 2}, 4), (std::vector<int>{}));
  EXPECT_EQ(window_maximum({4, 1, 2}, 0), (std::vector<int>{}));
  EXPECT_EQ(window_maximum({}, 2), (std::vector<int>{}));
  EXPECT_EQ(window_maximum({5, 5, 5}, 2), (std::vector<int>{5, 5}));
}

TEST(SlidingWindowTest, WindowSumsRolling) {
  EXPECT_EQ(window_sums({1, 2, 3, 4, 5}, 2),
            (std::vector<std::int64_t>{3, 5, 7, 9}));
  EXPECT_EQ(window_sums({1, 2, 3, 4, 5}, 3),
            (std::vector<std::int64_t>{6, 9, 12}));
  EXPECT_EQ(window_sums({1, 2, 3}, 4), (std::vector<std::int64_t>({})));
}

TEST(SlidingWindowTest, WindowSumsNoOverflow) {
  // large ints summed as int64: the naive int32 run would overflow
  const std::vector<int> big(64, 2'000'000'000);
  const auto sums = window_sums(big, 40);
  ASSERT_EQ(sums.size(), 25u);
  EXPECT_EQ(sums.front(), std::int64_t{40} * 2'000'000'000);
}

TEST(SlidingWindowTest, MaxAverageWindow) {
  EXPECT_NEAR(max_average_window({1, 12, -5, -6, 50, 3}, 4), 12.75, 1e-9);
  EXPECT_NEAR(max_average_window({1, 2, 3, 4}, 2), 3.5, 1e-9);
  EXPECT_NEAR(max_average_window({1, 2, 3}, 3), 2.0, 1e-9);
  EXPECT_EQ(max_average_window({1, 2, 3}, 5), 0.0);
  EXPECT_THROW(max_average_window({1, 2, 3}, 0), std::invalid_argument);
}

TEST(SlidingWindowTest, MinSubarrayLenClassic) {
  EXPECT_EQ(min_subarray_len({2, 3, 1, 2, 4, 3}, 7), 2u);  // [4,3]
  EXPECT_EQ(min_subarray_len({1, 4, 4}, 4), 1u);
  EXPECT_EQ(min_subarray_len({1, 2, 3, 4, 5}, 11), 3u);    // [3,4,5] or [2,4,5]
  EXPECT_EQ(min_subarray_len({1}, 1), 1u);
}

TEST(SlidingWindowTest, MinSubarrayLenNoneOrZeros) {
  EXPECT_EQ(min_subarray_len({1, 2, 3}, 100), 0u);
  EXPECT_EQ(min_subarray_len({}, 5), 0u);
  EXPECT_EQ(min_subarray_len({1, 2, 3}, 0), 1u);  // single element qualifies
}

TEST(SlidingWindowTest, LongestDistinctRun) {
  EXPECT_EQ(longest_distinct_run({1, 2, 1, 2, 3}, 2), 4u);   // [1,2,1,2]
  EXPECT_EQ(longest_distinct_run({1, 2, 3, 4}, 1), 1u);
  EXPECT_EQ(longest_distinct_run({5, 5, 5, 5}, 1), 4u);
  EXPECT_EQ(longest_distinct_run({1, 2, 1, 3, 2, 1}, 2), 3u);
  EXPECT_EQ(longest_distinct_run({1, 2, 3, 4}, 5), 4u);      // k >= n
  EXPECT_EQ(longest_distinct_run({1, 2, 3}, 0), 0u);
  EXPECT_EQ(longest_distinct_run({}, 2), 0u);
}