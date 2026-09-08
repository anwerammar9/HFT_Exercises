# Exercise concurrency/10_spin_barrier (ex28) — Spin Barrier (Reference Solution)

**What you implement:** a reusable sense-reversing spin barrier on two atomics
that (a) holds threads until all `n` arrive in a round, (b) publishes
pre-`wait()` writes to all threads that pass the same round, and (c) resets
cleanly for the next round.

**Approach**
- Members: `n_` (fixed size), `count_` (arrivals in the current round),
  `generation_` (round counter).
- `wait()`:
  1. `round = generation_.load(relaxed)` — the round this thread checks in for.
  2. `arrived = count_.fetch_add(1, acq_rel) + 1`.
  3. If `arrived == n_` → this thread is **the last arriver**: reset `count_`
     to 0 (next round starts fresh) and `generation_.store(round + 1, release)`
     (flips everyone's sense so they all proceed). No extra flag-poke to the
     losers is needed — they read the exact thing we write.
  4. Else spin: `while (generation_.load(acquire) == round) yield();`.
- Ordering: the last arriver's `release` store pairs with each waiter's
  `acquire` load — that expression publishes every non-last thread's
  pre-`wait()` writes (each also published theirs by their own `fetch_add`,
  `acq_rel` closes the chain).
- Reuse is free: the counter is reset by the last arriver *before* the round
  advances, so a lapping thread of a subsequent round arrives on a clean `0`.

## Reference API — `include/spin_barrier.h`

```cpp
#ifndef EXERCISE28_SPIN_BARRIER_H_
#define EXERCISE28_SPIN_BARRIER_H_

#include <atomic>
#include <cstddef>

// A reusable sense-reversing spin barrier (hand-rolled, no std::barrier).
//
// A barrier is the rendezvous of parallel programs: N threads must ALL reach
// wait() before ANY of them may pass. This one busy-waits on two atomics
// instead of sleeping on a condition variable, so on the fast path it never
// enters the kernel — the coordination point for tight numerics/reduction
// loops where the wait is expected to be short.
//
// Contract:
//   - wait(): blocks (spinning) until all N registered threads have arrived,
//     then lets them ALL proceed. The barrier is REUSABLE: once a round
//     completes, the same instance synchronizes the next round.
//   - generation(): starts at 0 and increments once per completed round.
//   - n == 0 cannot work (wait() would never release) => the constructor
//     throws std::invalid_argument.
//
// Correctness note (why it works):
//   Every thread records the round number it arrived in, then atomically
//   increments a shared arrival counter. The LAST arriver (the one that bumps
//   the counter to exactly n) resets the counter and advances the round; every
//   other thread simply spins until the round advances. No thread can pass a
//   round until all n have checked in for it.
//
// TODO(anwer): implement wait() in src/spin_barrier.cpp.
//
//   std::size_t round = generation_.load(relaxed);
//   std::size_t arrived = count_.fetch_add(1, acq_rel) + 1;
//   if (arrived == n_) {                  // I AM the last arriver
//     count_.store(0, relaxed);           // reset for the next round
//     generation_.store(round + 1, release);  // release everyone else
//   } else {
//     while (generation_.load(acquire) == round) std::this_thread::yield();
//   }

class SpinBarrier {
 public:
  explicit SpinBarrier(std::size_t n);

  SpinBarrier(const SpinBarrier&) = delete;
  SpinBarrier& operator=(const SpinBarrier&) = delete;

  void wait();
  std::size_t generation() const noexcept;

 private:
  std::size_t n_;
  std::atomic<std::size_t> count_{0};
  std::atomic<std::size_t> generation_{0};
};

#endif  // EXERCISE28_SPIN_BARRIER_H_
```

## Reference implementation — `src/spin_barrier.cpp`

```cpp
#include "spin_barrier.h"

#include <stdexcept>
#include <thread>

SpinBarrier::SpinBarrier(std::size_t n) : n_(n) {
  if (n == 0) throw std::invalid_argument("SpinBarrier size must be > 0");
}

void SpinBarrier::wait() {
  const std::size_t round = generation_.load(std::memory_order_relaxed);
  const std::size_t arrived =
      count_.fetch_add(1, std::memory_order_acq_rel) + 1;
  if (arrived == n_) {
    count_.store(0, std::memory_order_relaxed);  // clean slate for round n+1
    generation_.store(round + 1, std::memory_order_release);  // release all
  } else {
    while (generation_.load(std::memory_order_acquire) == round) {
      std::this_thread::yield();
    }
  }
}

std::size_t SpinBarrier::generation() const noexcept {
  return generation_.load(std::memory_order_acquire);
}
```

**How the tests verify you:** `WaitBlocksUntilAllNThreadsArrive` sleeps then
asserts zero threads passed — instant red against a no-op stub.
`AllThreadsSeeFullArrivalAfterRelease` proves the release ordering: each thread
must observe all N pre-`wait()` increments once past the barrier.
`ReusableAcrossMultipleRounds` runs 5 rounds and checks `generation() == 5`
(a stub returns 0 → red). All green under TSan as well (`build-tsan`).