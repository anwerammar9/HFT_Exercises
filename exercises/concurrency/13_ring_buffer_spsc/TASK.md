# Exercise 13 — SPSC Lock-Free Ring Buffer (Task)

## The problem (in plain words)

One producer thread and one consumer thread need a fast FIFO with **absolutely
no locks**. The trick that makes it lock-free: `head`/`tail` are
**monotonically increasing counters that never wrap** (wrapping is done on the
*index* with `idx & (N−1)`, never on the counters). One slot stays empty on
purpose so `head == tail` unambiguously means EMPTY and
`tail − head == N−1` means FULL.

## Requirements (what the tests check)

1. Usable capacity = **N−1** (the array holds `N` slots, `N` a power of two;
   index computed as `pos & (N−1)` instead of `%`).
2. FIFO order; `size()`/`empty()`/`full()` are exact at the boundaries
   (including the wrap-around case).
3. `try_push(v)` returns `false` when full — **nothing stored**.
4. `try_pop(out)` returns `false` when empty — **`out` untouched**.
5. **The concurrency test:** one producer pushes `0..M−1`, one consumer pops —
   it must receive exactly `0..M−1` in order, no loss, no duplication, no
   crash.
6. Runs clean under TSan (`tsan;stress`): the value write must be published
   with a **release** store and read with an **acquire** load — TSan flags the
   wrong ordering.

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

The stub lives in `include/ring_buffer_spsc.h` (a template) — implement inline.

## How to think about it (suggested design)

- `head_` (owner: consumer) and `tail_` (owner: producer), both
  `alignas(64)` atomics so they never share a cache line.
- **Producer** (`try_push`): full-check (`tail_ − head_ == kCapacity`), move
  the value into slot `tail_ & (N−1)`, then release-increment `tail_`.
- **Consumer** (`try_pop`): empty-check (`head_ == tail_`), read slot
  `head_ & (N−1)`, then release-increment `head_`.
- Because each counter is **written by exactly one thread**, there is no CAS —
  just `load` of the *other* side's counter + `store` of your own. That is why
  it's lock-free *and* simple.
- `size()` = `tail_ − head_`.

## Make it harder (optional — not covered by the tests)

- **Peek:** a `peek(int offset)` that reads the i-th item without consuming it
  (a "window" for back-pressure decisions).
- **Batch APIs:** `push_batch(it, it)` and `pop_batch(dst, max)` that transfer
  several elements with a single `store`/`load`.
- **Drop-quota slot:** a variant that keeps a per-item `loss counter` when the
  producer is told "drop if full", used by market-data fan-out.
- **Byte-oriented:** a fixed-size byte buffer SPSC (used for UDP packet
  assembly) with a `put(bytes, len)`/`get(dst, cap)` API, benchmarked against
  the full-duplex version.

## Files

- Stub: `include/ring_buffer_spsc.h` (template — implement inline)
- Tests: `test/test_ring_buffer_spsc.cpp`
- Reference: `SOLUTION.md`