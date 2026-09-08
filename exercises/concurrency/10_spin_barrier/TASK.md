# Exercise concurrency/10_spin_barrier (ex28) — Spin Barrier (Task)

## The problem (in plain words)

A **barrier** is the rendezvous point of parallel programs: `N` threads must
*all* arrive at `wait()` before *any* of them may leave it. This one is a
**spinning** barrier — no condition variable, no sleep, just two atomics — the
style used inside tight numerics/reduction loops where the waiting cost is
expected to be a few nanoseconds, not a kernel trip.

## Requirements (what the tests check)

1. `SpinBarrier(n)` with `n == 0` is rejected: `wait()` could never release,
   so the constructor throws `std::invalid_argument`.
2. `wait()` blocks until all `n` threads have arrived; then **all** of them
   proceed. A thread must not pass a round until `n` arrivals have happened
   **in that round**.
3. **Release ordering:** everything a thread did *before* `wait()` must be
   visible to every thread *after* `wait()` on the same round (this is the
   memory-order guarantee the tests check with ticket counters).
4. **Reusable:** after a round completes the same barrier synchronizes the
   next round; `generation()` starts at 0 and increments once per completed
   round.
5. No `std::barrier` / `std::latch` / `std::condition_variable` — hand-rolled
   atomics only.
6. **The discriminator:** threads reaching `wait()` early must demonstrably
   stay blocked until the full set has arrived (a no-op stub fails this red).

## Public API

```cpp
class SpinBarrier {
  explicit SpinBarrier(std::size_t n);  // n == 0 throws std::invalid_argument
  void wait();
  std::size_t generation() const noexcept;
 private:
  std::size_t n_;
  std::atomic<std::size_t> count_{0};
  std::atomic<std::size_t> generation_{0};
};
```

## How to think about it (suggested design)

- **Sense-reversing barrier:** every thread records the round number it arrived
  in, atomically bumps the arrival counter, and:
  - if it's **not** the last arriver → spin until `generation_` advances
    (its *sense* flips);
  - if it **is** the last (the counter reached exactly `n`) → reset the
    counter for the next round and `store` the incremented `generation_`,
    releasing everyone.
- Memory ordering that makes the tests pass while staying correct:
  - `count_.fetch_add(1, acquire_release)` — the arrival is both a publish
    (your pre-round writes) and a consume (your wait spin).
  - last arriver: `count_.store(0, relaxed)` (next round's arrivals are fresh
    RMWs anyway), then `generation_.store(g+1, release)`.
  - waiters: `generation_.load(acquire)` in the spin, `yield()` between polls
    so a spinning core isn't a busy-burner.
- **Don't** read `generation()` inside a wait loop or record it "just after"
  passing — a lazily-lapped last arriver can already be in the next round.
  The tests track per-thread round counters instead.

## Make it harder (optional — not covered by the tests)

- **Cost / contention:** micro-benchmark the barrier (a) with `yield()` in the
  spin, (b) with a `pause`-style backoff via `__builtin_ia32_pause`, (c) with
  `std::thread::yield` removed, over 2/4/8/16 threads × 10⁶ rounds. Where does
  the cache-line ping-pong on `count_` show up?
- **Centralized vs. combining tree:** implement a **tree barrier** (threads
  rendezvous pairwise up a binary tree, `O(log N)` RMWs per round instead of a
  single hot counter) and compare.
- **`std::experimental`-free phaser:** extend to a split-phase barrier
  (arrive-and-wait then arrive-and-go) for pipeline-style stencil loops.
- **TSan stress:** run the suite under `build-tsan` on many cores and confirm
  zero data-race reports from your ordering choices.

## Files

- Stub: `src/spin_barrier.cpp`
- Tests: `test/test_spin_barrier.cpp`
- Reference: `SOLUTION.md`