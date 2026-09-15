# Exercise concurrency/01_spinlock (ex11) — Spinlock (Task)

## The problem (in plain words)

A **spinlock** is the smallest lock in the toolkit: threads busy-wait on an
atomic flag instead of sleeping. Naively spinning on an `exchange` stampedes the
shared cache line, so production spinlocks use **test-and-test-and-set (TTAS)**:
first *read* the flag cheaply, only attempt the atomic swap when it looks free;
and they **back off** under contention so the line can be released. The lock must
satisfy C++'s `BasicLockable` contract so `std::lock_guard` /
`std::scoped_lock` work on it directly.

## Requirements (what the tests check)

1. `lock()` blocks until the lock is acquired. No fairness required (some
   thread may be unlucky).
2. `unlock()` releases the lock; it must be called by the holder. Releasing
   without holding is UB (same as `std::mutex`).
3. `try_lock()` returns `true` iff it acquired the lock **without blocking**;
   `false` while somebody else holds it.
4. **The discriminator:** N threads each increment a shared *non-atomic*
   counter M times under the lock — the final value must be exactly `N·M`.
   (A broken or missing lock fails this test.)
5. **Liveness:** under prolonged contention every thread still makes progress —
   the backoff must actually back off (bounded-time test), otherwise the
   lock-starves or the test times out.
6. Not recursive — a second `lock()` on the same thread deadlocks (that's
   expected, don't fix it).
7. Works wrapped in `std::lock_guard<SpinLock>`.

## Public API

```cpp
class SpinLock {
  void lock();
  void unlock();
  bool try_lock();
 private:
  std::atomic<bool> state_{false};
};
```

## How to think about it (suggested design)

- TTAS on `state_` (`false` = free, `true` = held):
  1. `lock()`: `while (state_.exchange(true))` **only** after observing the
     flag is free — i.e. spin on `while (state_.load())` (a cheap read), then
     attempt the swap when it turns `false`. That one-two stops the cache-line
     ping-pong.
  2. `try_lock()`: a single `exchange` that returns the *previous* value —
     `false` means we won.
  3. `unlock()`: plain `store(false)`.
- **Backoff:** when the lock is contended, start with
  `std::this_thread::yield()`, then bounded `std::this_thread::sleep_for`
  doubling up to a cap (~512 µs). Reset the backoff after a successful acquire.
- `seq_cst` is fine at this scale; tighten to acquire/release **only** if you
  can explain exactly why they suffice (the tests don't demand it).

## Make it harder (optional — not covered by the tests)

- **Ticket lock:** implement a second `TicketLock` (two counters: take a
  ticket, wait until your number is served) and micro-benchmark fairness vs.
  the spinlock.
- **`try_lock_for`/`try_lock_until`:** add timed try variants with a deadline.
- **Benchmark:** build `spinlock_bench` if available and compare TTAS-with-
  backoff against plain TAS and against `std::mutex` over n threads.
- **Counters:** track `acquisition_spins` and `backoff_sleeps` (atomics) and
  tune the cap against the benchmark.

## Files

- Stub: `src/spinlock.cpp`
- Tests: `test/test_spinlock.cpp`
- Reference: `SOLUTION.md`