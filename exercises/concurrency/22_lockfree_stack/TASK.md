# Exercise 22 — Lock-Free Stack (Treiber) (Task)

## The problem (in plain words)

A **Treiber stack** is the simplest lock-free structure: a singly-linked list
whose head is claimed with a CAS, usable by any number of concurrent
producers/consumers. Its famous bug is the **ABA problem** — a node can be
popped, recycled, and pushed back *between* your CAS load and CAS attempt, so
`compare_exchange` succeeds against a node that is no longer the one you read.
Handling and **documenting** that choice is a mandatory part of the exercise.

## Requirements (what the tests check)

1. Multiple concurrent producers + consumers (all through the same stack).
2. `push(T)`: no failure mode (allocates a fresh node — Treiber style).
3. `try_pop(T&)`: `false` when empty; the **out-parameter is untouched** on
   empty.
4. **The concurrency test:** N threads each push M items while M·N/2 threads
   concurrently pop — total popped == total pushed, no crashes, no UB; runs
   clean under TSan (`tsan;stress`).
5. Single-thread **LIFO** ordering correctness (last pushed pops first).
6. The chosen **ABA strategy is documented in a comment** — reviewers check that
   you can name how a recycled address could corrupt the CAS and exactly how
   your scheme prevents it.

## Public API

```cpp
template <typename T>
class LockFreeStack {
  void push(T value);
  bool try_pop(T& out);
};
```

The stub lives in `include/lockfree_stack.h` (a template) — implement inline.

## How to think about it (suggested design)

- `head_` is an `atomic<Node*>`; `push` allocates a `Node`, then a
  `compare_exchange_weak` retry loop; `try_pop` reads `head→next` and CAS-swaps
  the head with retries.
- **ABA strategy (this exercise's chosen contract):** nodes are **never
  reclaimed while the stack lives**. Popped nodes park on an internal **retired
  list** (also never returned to the allocator) until the whole stack is
  destroyed. A node's fields are never written again after a pop, so a stale
  reader racing read-only on a popped node is safe. That makes the address
  reuse that causes ABA impossible — document this argument in a comment.
- The destructor loops that free both lists are already written in the header;
  you add `push`/`try_pop` and the `retired_` insertion.
- Note: a *production* implementation would replace retire-with-allocator-free
  with hazard pointers or epoch reclamation — say so in a comment.

## Make it harder (optional — not covered by the tests)

- **Hazard pointers:** implement hazard-pointer-based reclamation so nodes are
  freed as soon as no thread can still dereference them — the real follow-up.
- **Bound the stack:** a fixed-capacity variant whose `push` returns `false`
  when full (reusing Exercise 23's pool for nodes), then stress it.
- **`size()`/`empty()`:** an approximate count and a non-blocking emptiness
  check, documented as racy-but-ok.
- **Prove LIFO under no concurrency:** a heavy single-threaded randomized
  push/pop test where the expected order is a reference `std::stack` compared
  operation-by-operation.
- **Batch drain:** `pop_n(dst, k)` that pops several at once with fewer CAS
  rounds (a known optimization) and a benchmark showing the win.

## Files

- Stub: `include/lockfree_stack.h` (template — implement inline)
- Tests: `test/test_lockfree_stack.cpp`
- Reference: `SOLUTION.md`