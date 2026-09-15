# Exercise concurrency/05_ring_buffer_mpmc (ex21) — Bounded MPMC Queue (Vyukov) (Task)

## The problem (in plain words)

A FIFO that many producers and many consumers share **with no lock anywhere**.
The Vyukov design gives each **slot** its own sequence number; producers and
consumers "claim" a slot's turn with a single CAS at the queue head/tail, so no
one ever contends for a queue-wide lock. `push` **blocks** (spins with backoff)
until a slot frees — never drops — and `try_pop` never blocks.

## Requirements (what the tests check)

1. FIFO, bounded, multiple producers + multiple consumers at the same time.
2. `push(const T&)` / `push(T&&)` are **blocking**: they spin (with backoff)
   until a slot can be claimed. A full queue is never a drop.
3. `try_pop(T&)` returns `false` immediately when empty (does not block) and
   leaves `out` untouched.
4. Usable capacity == **exactly** `capacity` — this design uses per-slot
   sequence numbers, *not* the "one empty slot" trick from Exercise 13.
5. **The concurrency test:** P producers each push M uniquely-tagged items; C
   consumers drain until all P·M items are out — no duplicates, no loss, every
   tag exactly once.
6. Runs clean under TSan (`tsan;stress`): the value must be **release-stored
   before** the store that publishes the slot.

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

The stub lives in `include/ring_buffer_mpmc.h` (a template) — implement inline.

## How to think about it (suggested design)

- `Slot { T value; alignas(64) std::atomic<std::size_t> seq; }` and **slot *i*
  starts with `seq == i`**. `tail_`/`head_` are plain monotonically increasing
  atomics; the slot index is `pos % N`.
- **push:** spin until `slots_[pos % N].seq == pos` (the slot is enqueue-ready),
  CAS-claim `tail_`, write the value, then **release-store** `seq = pos+1`
  (the slot is now dequeue-ready).
- **try_pop:** mirror with `head_` — expect `seq == pos+1`, call it yours with a
  CAS, read the value, finish by **release-storing** `seq = pos+N` (so the slot
  cycles back to a future producer). If `pos − head_` reads less than queued
  items → empty → `false`.
- The sequence-numbers-wrapping trick is the whole exercise; the release/acquire
  ordering is what keeps TSan quiet.

## Make it harder (optional — not covered by the tests)

- **`try_push`:** a non-blocking enqueue variant that returns `false` when full
  instead of spinning — then benchmark blocking vs. try-based consumers.
- **Backoff tuning:** exponential `yield`/`sleep` backoff on the spin loops and
  a throughput table over producer/consumer counts.
- **`size()` approximation:** an unsynchronized `tail_ − head_` (documented as
  approximate, and why it's safe enough for metrics).
- **Intrusive nodes:** a variant storing `unique_ptr<T>` so small payloads skip
  a copy on push, plus a `drain()` that pops everything into a vector.
- **Explain the invariant in comments:** write out *why* `seq` bumping from
  `pos` → `pos+1` → `pos+N` guarantees exactly one holder of a slot at a time;
  a reviewer should be able to follow your argument line by line.

## Files

- Stub: `include/ring_buffer_mpmc.h` (template — implement inline)
- Tests: `test/test_ring_buffer_mpmc.cpp`
- Reference: `SOLUTION.md`