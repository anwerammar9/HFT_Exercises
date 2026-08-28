# Exercise 21 — Bounded MPMC Queue (Vyukov) (Task)

## Problem
A bounded multi-producer/multi-consumer FIFO with **no global lock**: each slot
carries its own sequence number, so producers/consumers claim slots with a CAS
and never contend for a queue-wide lock.

## Requirements (what the tests check)
1. FIFO, bounded, multiple producers + multiple consumers simultaneously.
2. `push(const T&)` / `push(T&&)` are **BLOCKING**: they spin (with backoff)
   until a slot frees. Never drop on a full queue.
3. `try_pop(T&)` returns `false` immediately when empty (never blocks) and
   leaves `out` untouched.
4. Usable capacity == exactly `capacity` (per-slot sequence numbers — NOT the
   "one empty slot" trick).
5. **The concurrency test**: P producers each push M uniquely-tagged items, C
   consumers drain until P·M items — no duplicates, no loss, every tag exactly
   once.
6. Runs clean under TSan (`tsan;stress`): the value must be release-stored
   before the releasing store that publishes the slot.

## Public API
```cpp
template <typename T>
class MpmcQueue {
  explicit MpmcQueue(std::size_t capacity);
  void push(const T& value);     // blocking (spin until a slot is claimed)
  void push(T&& value);          // blocking
  bool try_pop(T& out);          // false when empty, never blocks
  std::size_t capacity() const;
};
```

## Design notes
Vyukov scheme with `Slot { T value; alignas(64) std::atomic<size_t> seq; }`:
slot *i* starts with `seq == i`. `push`: wait until
`slots_[pos % N].seq == pos`, CAS-claim `tail_`, write the value, then
release-store `seq = pos+1`. `try_pop` mirrors with `head_` (expect
`seq == pos+1`, finish with `seq = pos+N`); `dif < 0` ⇒ empty ⇒ false.

## Files
- Stub: `include/ring_buffer_mpmc.h` (template — implement inline)
- Tests: `test/test_ring_buffer_mpmc.cpp`
- Reference: `SOLUTION.md`