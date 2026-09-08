# Exercise concurrency/11_mcs_lock (ex29) — MCS Queue Lock (Task)

## The problem (in plain words)

The **MCS lock** is the fair fix for Exercise 11's spinlock. A spinlock makes
every waiter hammer one shared atomic — that's cache-line ping-pong, starvation
risk and no ordering guarantee. MCS instead forms a **FIFO queue of waiting
threads**: each thread spins on a flag it *owns itself*, and the holder hands
the lock directly to the thread behind it. No one polls anyone else's cache
line, throughput stays flat as cores scale, and hand-off order is
deterministic.

## Requirements (what the tests check)

1. API is `lock(Waiter&)` / `unlock(Waiter&)`: the caller supplies a **Waiter
   node** for its thread. Distinguishing feature — there's no "the lock is
   free" bit; the queue is the state.
2. **Mutual exclusion:** N threads, each with its own node, increment a shared
   *non-atomic* counter M times under the lock → exactly `N·M` at the end. (A
   no-op lock fails this red.)
3. **At most one holder:** with multiple threads contending, the count of
   threads simultaneously inside the critical section must never exceed 1.
4. **Liveness:** every thread completes its bounded work under contention in
   bounded time — the queue hand-off must never livelock.
5. **Node reuse:** a single `Waiter` must work across many lock/unlock rounds
   (reused on the same thread) — the standard usage pattern.
6. Unlock hands off to the *next* thread in FIFO order (that subtle
   `next_ == nullptr` fast-path is exactly what the CAS-with-`yield()` dance
   handles).

## Public API

```cpp
class McsLock {
 public:
  class Waiter {
    Waiter() = default;
    // private, friend class McsLock: std::atomic<bool> locked_{true};
    //                              std::atomic<Waiter*> next_{nullptr};
  };
  void lock(Waiter& w);
  void unlock(Waiter& w);
 private:
  std::atomic<Waiter*> tail_{nullptr};
};
```

## How to think about it (suggested design)

- `tail_` is the queue tail: a thread enqueues itself with
  `tail_.exchange(&w, acq_rel)` and gets the *previous* tail back.
  - If the previous tail was `nullptr` → the queue was empty; you just won the
    lock (your node is already the new tail).
  - Otherwise → you're someone's successor: publish yourself on their
    `next_` (release), then spin on your own `locked_` (acquire) until they
    let you in.
- `unlock(w)`: hand the lock onward.
  - If `w.next_ == nullptr` (looks like no successor): try to move `tail_`
    from `&w` to `nullptr` (CAS). Win → queue empty, done. Lose → a successor
    has *just* linked behind you; yield-spin until `next_` appears, then
    release their `locked_`.
  - If a successor exists: release `w.next_->locked_` directly.
- Every waiter spins on **its own** `locked_`, so the hand-off touches exactly
  two cache lines (my flag + my neighbor's flag) instead of one hot shared bit.
- Ordering rules of thumb: publish to *your* chain with `release`, observe
  your *own* flag with `acquire`.

## Make it harder (optional — not covered by the tests)

- **CLH lock:** a simpler cousin of MCS that spins on the *predecessor's* node
  (no `next_` pointer, but `tail_` CAS is on every unlock). Implement a
  `ClhLock`, compare fairness and contention against MCS under TSan.
- **`lock_guard`-compatible wrapper:** give `McsLock` a `std::scoped_lock`
  shim (a `Waiter` attached to the scope) so it composes with `std::lock_guard
  <McsLock>`.
- **Backoff in the spin loop:** add bounded `yield()`/`pause` backoff inside
  the MCS wait-spins to save power under high contention; micro-benchmark
  cycles-per-acquire at 1/2/4/8/16 threads.
- **Multiplex one node lost-lock flag:** eliminate the acquire-phase flag by
  making `Waiter::locked_` mean "I have the lock" and `unlock` clear *its own*
  successor's flag — trace what that changes for the memory ordering.

## Files

- Stub: `src/mcs_lock.cpp`
- Tests: `test/test_mcs_lock.cpp`
- Reference: `SOLUTION.md`