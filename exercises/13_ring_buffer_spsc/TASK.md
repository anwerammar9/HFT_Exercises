# Exercise 13 — SPSC Lock-Free Ring Buffer (Task)

## Problem
A single-producer/single-consumer lock-free ring buffer over a fixed array:
`head`/`tail` are **monotonically increasing counters** (never wrapped — that's
what makes it lock-free), one slot stays empty so `head == tail` unambiguously
means EMPTY.

## Requirements (what the tests check)
1. Usable capacity = **N−1** (array size N, a power of two; wrap via
   `idx & (N−1)`).
2. FIFO order; correct `size()`/`empty()`/`full()` at the boundaries.
3. `try_push(v)` returns `false` when full (nothing stored).
4. `try_pop(out)` returns `false` when empty (out untouched).
5. **The concurrency test**: one producer pushes 0..M−1, one consumer pops —
   receives exactly 0..M−1 in order, no loss/duplication/crash.
6. Runs clean under TSan (`tsan;stress`): the payload write must be published
   with a **release** store and consumed with an **acquire** load.

## Public API
```cpp
template <typename T, std::size_t N>  // N must be a power of two
class SpscRingBuffer {
  static constexpr std::size_t kCapacity = N - 1;
  bool try_push(T value);
  bool try_pop(T& out);
  std::size_t size() const;
  bool empty() const;
  bool full() const;
};
```

## Design notes
`head_` (consumer) and `tail_` (producer) are `alignas(64)` atomics; slot at
`idx & (N−1)`. Producer: full-check → move value into slot → release `tail_`.
Consumer: empty-check → read slot → release `head_`. Exactly one producer and
one consumer; anything else is UB.

## Files
- Stub: `include/ring_buffer_spsc.h` (template — implement inline)
- Tests: `test/test_ring_buffer_spsc.cpp`
- Reference: `SOLUTION.md`