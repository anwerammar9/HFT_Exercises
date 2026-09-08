#include "two_pointer.h"

// TODO(anwer): implement the two-pointer loops (see SOLUTION.md).
//
// Suggested shape:
//   - has_sum_pair: advance lo when the sum is short, retreat hi when long.
//   - has_sum_pair(a,b): parallel pointers, b walks backwards (both sorted).
//   - merge_sorted: take the smaller front until both drains finish.
//   - remove_duplicates: slow writer + fast reader, return the new size.
//   - has_triple_sum: sort a copy, fix the first element, inner two-pointer.
//   - max_area: opposing pointers, advance the shorter bar.
//   - is_palindrome: opposing pointers, mismatch -> false.
//
// Stub: every probe answers false, merges/dedups return empty/0, so the
// positive-path tests run RED without hanging or crashing.

namespace two_pointer {

bool has_sum_pair(const std::vector<int>& /*sorted*/, int /*target*/) {
  return false;
}

bool has_sum_pair(const std::vector<int>& /*a*/, const std::vector<int>& /*b*/,
                  int /*target*/) {
  return false;
}

std::vector<int> merge_sorted(const std::vector<int>& /*a*/,
                              const std::vector<int>& /*b*/) {
  return {};
}

std::size_t remove_duplicates(std::vector<int>& /*sorted*/) { return 0; }

bool has_triple_sum(const std::vector<int>& /*values*/, int /*target*/) {
  return false;
}

int max_area(const std::vector<int>& /*heights*/) { return 0; }

bool is_palindrome(const std::string& /*text*/) { return false; }

}  // namespace two_pointer