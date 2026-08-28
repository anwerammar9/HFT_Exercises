# Exercise 11 — Spinlock (Task)

## Problem
A test-and-test-and-set (TTAS) spinlock with exponential backoff that satisfies
C++ `BasicLockable`, so `std::lock_guard` / `std::unique_lock` / `std::scoped_lock`
work directly on it.

## Requirements (what the tests check)
1. `lock()` blocks until acquired; no fairness required.
2. `unlock()` releases; must be called by the holder. Releasing without holding
   is UB (like `std::mutex`).
3. `try_lock()` returns `true` iff the lock was acquired **without blocking**;
   returns false while held.
4. **The discriminator**: N threads increment a shared non-atomic counter M
   times each under the lock — final value must equal N·M.
5. Liveness: all threads eventually acquire under prolonged contention — the
   backoff must actually back off (bounded-time test).
6. Not recursive.
7. Works with `std::lock_guard<SpinLock>`.

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

## Design notes
Spin on the atomic with TTAS (avoid cache-line ping-pong): read the flag while
held, only TAS when you see it free. On contention back off exponentially —
`std::this_thread::yield()` first, then bounded `sleep_for` doubling up to a
cap (~512 µs). `seq_cst` is fine at this scale; tighten to acquire/release if
you can defend it.

## Files
- Stub: `src/spinlock.cpp`
- Tests: `test/test_spinlock.cpp`
- Reference: `SOLUTION.md`