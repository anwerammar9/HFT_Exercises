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