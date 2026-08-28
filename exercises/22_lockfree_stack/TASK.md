# Exercise 22 — Lock-Free Stack (Treiber) (Task)

## Problem
A Treiber stack: a singly-linked list claimed by CAS on the head pointer,
usable by multiple concurrent producers and consumers. The **ABA problem must
be handled and documented** — this is the classic follow-up question.

## Requirements (what the tests check)
1. Multiple concurrent producers + consumers.
2. `push(T)`: no failure mode (allocates a fresh node).
3. `try_pop(T&)`: `false` when empty; the out-parameter is **untouched** on
   empty.
4. **The concurrency test**: N threads each push M items while M·N/2 threads
   concurrently pop; total popped == total pushed; no crashes/UB; runs clean
   under TSan (`tsan;stress`).
5. Single-thread LIFO ordering correctness.
6. The chosen **ABA strategy is documented in a comment** — reviewers check
   that you know how a recycled address could corrupt the CAS and how your
   scheme prevents it.

## Public API
```cpp
template <typename T>
class LockFreeStack {
  void push(T value);
  bool try_pop(T& out);
};
```

## Design notes
CAS retry loop on `head_` (`compare_exchange_weak`). ABA strategy already
chosen by this exercise's contract: **nodes are never reclaimed during
operation** — popped nodes park on an internal retired list (also never
returned to the allocator) until the whole stack is destroyed. A node's fields
are never written again after a pop, so a stale reader racing read-only on a
popped node is safe. The destroy loops are already implemented in the header.

## Files
- Stub: `include/lockfree_stack.h` (template — implement inline)
- Tests: `test/test_lockfree_stack.cpp`
- Reference: `SOLUTION.md`