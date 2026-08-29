# Exercise 39 — Two-Pointer Patterns (Reference Solution)

**What you implement:** the five canonical two-pointer patterns as free
functions wearing an HFT hat — matching an offsetting order in one sorted book
(`has_sum_pair`), splitting one order across two sorted feeds
(`has_sum_pair(a,b)`), merging two sorted streams (`merge_sorted`), de-duping
trade prints in place (`remove_duplicates`), sizing a three-instrument basket
(`has_triple_sum`), the widest spread between two converging levels
(`max_area`), and a symmetric-book sanity check (`is_palindrome`).

**Approach**
- **has_sum_pair(sorted, target):** opposing pointers `lo`/`hi`; if the sum is
  short advance `lo`, if it is long retreat `hi` — O(n), no extra space. The
  two DISTINCT positions are guaranteed by `lo < hi`.
- **has_sum_pair(a, b, target):** parallel pointers — `a` ascending from the
  front, `b` descending from the back (both inputs ascending); sums below target
  move through `a`, above target back through `b`.
- **merge_sorted:** one pass with `i`/`j`, take the smaller front element
  (`<=` keeps a-then-b order for ties), then drain the remainder — O(n+m).
- **remove_duplicates:** slow writer pointer + fast reader pointer; the writer
  only advances when the next value differs from what it last wrote, returning
  the new logical size. Tail contents beyond it are explicitly unspecified.
- **has_triple_sum:** sort a COPY (rich-order input is fine), fix the first
  element, then run the two-pointer pair sweep on the tail — O(n²) time,
  O(n) space for the copy.
- **max_area:** opposing pointers start at max width and advance the *shorter*
  bar inward (only that direction can ever beat the current best).
- **is_palindrome:** opposing pointers, mismatch → false; strict (case and
  spaces matter), documented in the contract.

## Reference API — `include/two_pointer.h`
#ifndef EXERCISE39_TWO_POINTER_H_
#define EXERCISE39_TWO_POINTER_H_

#include <cstddef>
#include <string>
#include <vector>

// Classic two-pointer patterns, straight from the interview canon, wearing an
// HFT hat: matching an offsetting order against the resting book (two-sum),
// merging two sorted feeds (merge), de-duping trade prints (remove_duplicates),
// sizing a three-instrument basket (has_triple_sum), the widest spread while
// two levels converge (max_area), and a symmetric-book sanity check
// (is_palindrome).
//
// Preconditions are part of the contract:
//   - has_sum_pair(sorted, ...) and remove_duplicates require ASCENDING input.
//   - min/max_area heights are non-negative.
//   - has_triple_sum sorts a COPY internally (its input order is free).
//   - is_palindrome is strict (case-sensitive, spaces count) — no filtering.
//
// TODO(anwer): implement the two-pointer loops (see SOLUTION.md). The stubs
// return defaults (false / empty / 0) so the positive-path tests run RED
// without hanging or crashing.

namespace two_pointer {

// Any two DISTINCT positions in `sorted` summing to `target`? Opposing
// pointers march in from both ends. NOT the same slot twice.
bool has_sum_pair(const std::vector<int>& sorted, int target);

// Any pair summing to `target` with ONE element from each sorted array?
// Parallel pointers: `a` ascending from the front, `b` descending from the
// back (both sorted ascending).
bool has_sum_pair(const std::vector<int>& a, const std::vector<int>& b,
                  int target);

// Merge two sorted arrays into one sorted array (equal values keep a-then-b
// order) — straight parallel-pointer walk.
std::vector<int> merge_sorted(const std::vector<int>& a,
                              const std::vector<int>& b);

// Remove duplicates in place from `sorted`; returns the new logical size.
// Elements beyond the returned size are unspecified. Slow/fast pointers.
std::size_t remove_duplicates(std::vector<int>& sorted);

// Any THREE values summing to `target`? Sorts a copy, fixes the first element,
// then runs the two-pointer inner sweep. O(n^2).
bool has_triple_sum(const std::vector<int>& values, int target);

// Largest rectangle volume between two vertical bars: (j-i)*min(h_i,h_j).
// Opposing pointers, moving the shorter bar inward. 0 for <2 bars.
int max_area(const std::vector<int>& heights);

// Strict palindrome test (case-sensitive, no character filtering).
bool is_palindrome(const std::string& text);

}  // namespace two_pointer

#endif  // EXERCISE39_TWO_POINTER_H_
## Reference implementation — `src/two_pointer.cpp`
#include "two_pointer.h"

#include <algorithm>

namespace two_pointer {

bool has_sum_pair(const std::vector<int>& sorted, int target) {
  if (sorted.size() < 2) return false;
  std::size_t lo = 0;
  std::size_t hi = sorted.size() - 1;
  while (lo < hi) {
    const long long sum = static_cast<long long>(sorted[lo]) + sorted[hi];
    if (sum == target) return true;
    if (sum < target)
      ++lo;
    else
      --hi;
  }
  return false;
}

bool has_sum_pair(const std::vector<int>& a, const std::vector<int>& b,
                  int target) {
  if (a.empty() || b.empty()) return false;
  std::size_t i = 0;
  std::size_t j = b.size() - 1;
  while (i < a.size()) {
    const long long sum = static_cast<long long>(a[i]) + b[j];
    if (sum == target) return true;
    if (sum < target) {
      ++i;
    } else {
      if (j == 0) break;
      --j;
    }
  }
  return false;
}

std::vector<int> merge_sorted(const std::vector<int>& a,
                              const std::vector<int>& b) {
  std::vector<int> out;
  out.reserve(a.size() + b.size());
  std::size_t i = 0, j = 0;
  while (i < a.size() && j < b.size()) {
    if (a[i] <= b[j]) {
      out.push_back(a[i++]);
    } else {
      out.push_back(b[j++]);
    }
  }
  while (i < a.size()) out.push_back(a[i++]);
  while (j < b.size()) out.push_back(b[j++]);
  return out;
}

std::size_t remove_duplicates(std::vector<int>& sorted) {
  if (sorted.empty()) return 0;
  std::size_t w = 1;
  for (std::size_t r = 1; r < sorted.size(); ++r) {
    if (sorted[r] != sorted[w - 1]) sorted[w++] = sorted[r];
  }
  return w;
}

bool has_triple_sum(const std::vector<int>& values, int target) {
  std::vector<int> v = values;
  std::sort(v.begin(), v.end());
  const std::size_t n = v.size();
  for (std::size_t i = 0; i + 2 < n; ++i) {
    std::size_t lo = i + 1;
    std::size_t hi = n - 1;
    while (lo < hi) {
      const long long sum = static_cast<long long>(v[i]) + v[lo] + v[hi];
      if (sum == target) return true;
      if (sum < target)
        ++lo;
      else
        --hi;
    }
  }
  return false;
}

int max_area(const std::vector<int>& heights) {
  const std::size_t n = heights.size();
  if (n < 2) return 0;
  std::size_t i = 0;
  std::size_t j = n - 1;
  int best = 0;
  while (i < j) {
    const int w = static_cast<int>(j - i);
    const int h = std::min(heights[i], heights[j]);
    best = std::max(best, w * h);
    if (heights[i] < heights[j])
      ++i;
    else
      --j;
  }
  return best;
}

bool is_palindrome(const std::string& text) {
  if (text.size() < 2) return true;
  std::size_t i = 0;
  std::size_t j = text.size() - 1;
  while (i < j) {
    if (text[i] != text[j]) return false;
    ++i;
    --j;
  }
  return true;
}

}  // namespace two_pointer