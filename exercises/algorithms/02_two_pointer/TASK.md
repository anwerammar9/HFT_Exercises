# Exercise algorithms/02_two_pointer (ex39) — Two-Pointer Patterns (Task)

## The problem (in plain words)

Many HFT tasks are "two sorted streams" problems — a bought order scanning
against a rising book, a spread leg matched against another feed — and the
case-cracker is the **two-pointer walk**: each pointer only ever moves one way,
so the whole thing is O(n) with O(1) extra space. Implement the canonical
patterns as free functions and reason about *when a pointer may move*.

## Requirements (what the tests check)

1. `has_sum_pair(sorted, target)` — sorted ascending, are there two
   **distinct** positions summing to `target`?
2. `has_sum_pair(a, b, target)` — cross-array pair: one value from `a`, one
   from `b`, both ascending.
3. `merge_sorted(a, b)` — merge two ascending vectors into one ascending
   vector; on ties `a`'s element must come first.
4. `remove_duplicates(sorted)` — in-place dedupe, returns the new logical size;
   the values left in the first `size` slots are the unique ones in order.
5. `has_triple_sum(values, target)` — three distinct elements summing to
   `target` (input is *not* sorted).
6. `max_area(heights)` — widest container: two bars i<j, area =
   `min(heights[i], heights[j]) * (j - i)`.
7. `is_palindrome(text)` — strict (case and spaces matter).

## Public API

```cpp
namespace two_pointer {
bool has_sum_pair(const std::vector<int>& sorted, int target);
bool has_sum_pair(const std::vector<int>& a, const std::vector<int>& b, int target);
std::vector<int> merge_sorted(const std::vector<int>& a, const std::vector<int>& b);
std::size_t remove_duplicates(std::vector<int>& sorted);
bool has_triple_sum(const std::vector<int>& values, int target);
int max_area(const std::vector<int>& heights);
bool is_palindrome(const std::string& text);
}
```

Stub in `src/two_pointer.cpp`, declarations in `include/two_pointer.h`.

## How to think about it (suggested design)

- `has_sum_pair`: `lo = 0`, `hi = n-1`; sum too small → `++lo`, too big →
  `--hi`; `lo < hi` guarantees the two positions are distinct.
- Cross-array: walk `a` forward from the front, `b` **backward** from the back.
- `merge_sorted`: compare front elements, `<=` keeps `a`'s on ties, drain both.
- `remove_duplicates`: a slow **writer** pointer that only advances when it sees
  a new value, a fast **reader** pointer scanning ahead.
- `has_triple_sum`: sort a copy, fix the first element, run the two-pointer
  sweep on the tail (O(n²)).
- `max_area`: start at max width; only advancing the **shorter** bar can beat
  the current best.
- `is_palindrome`: opposing pointers, mismatch → false.

## Make it harder (optional — not covered by the tests)

- **`count_sum_pairs`:** return the *count* of distinct pairs summing to
  `target` (handle duplicates correctly).
- **Trapping rain water:** given a profile, compute water retained — a
  two-pointer algorithm that runs in O(n) with O(1) memory.
- **`closest_pair`:** from two sorted arrays, the pair whose sum is closest to a
  target (the "quote spread to a target notional" form).
- **`k_empty_slots`:** the classic "k empty slots" two-pointer sliding problem.

## Files

- Stub: `src/two_pointer.cpp`
- Tests: `test/test_two_pointer.cpp`
- Reference: `SOLUTION.md`
