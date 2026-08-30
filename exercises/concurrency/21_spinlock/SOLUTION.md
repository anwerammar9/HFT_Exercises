# Exercise 11 — Spinlock (Reference Solution)

**What you implement:** a test-and-test-and-set (TTAS) spinlock with backoff that
satisfies C++ `BasicLockable`, so `std::lock_guard`/`unique_lock` work and the
mutual-exclusion counter test passes.

**Approach**
- `lock()`: `exchange(state_ true)` claims the lock; on failure spin on a plain
  `load` (test-and-test-and-set — a hairpin spin keeps the line shared and
  livelocks), then `yield()`. Every 16 failed attempts, back off with
  `sleep_for(nanoseconds(attempts))`, doubling in a bounded way — the liveness
  test is the discriminator.
- `unlock()`: release `store(false)`; the release store publishes the critical
  section.
- `try_lock()`: one non-blocking `exchange`; `false` iff already held.
- Memory ordering: acquire/release is enough for a mutex (no seq_cst needed).

## Reference API — `include/spinlock.h`

```cpp
#pragma once

#include <atomic>

// TODO(anwer): implement a spinlock on `state_` (see SOLUTION.md).
//
// The lock must satisfy C++ BasicLockable so that std::lock_guard works, and
// must remain livelock-free under contention (the liveness test enforces it).
class SpinLock {
 public:
  SpinLock() = default;

  SpinLock(const SpinLock&) = delete;
  SpinLock& operator=(const SpinLock&) = delete;

  void lock();      // blocks until the lock is acquired
  bool try_lock();  // true iff acquired without blocking
  void unlock();    // releases the lock

 private:
  std::atomic<bool> state_{false};
};
```

## Reference implementation — `src/spinlock.cpp`

```cpp
#include "spinlock.h"

#include <thread>

void SpinLock::lock() {
  for (size_t attempts = 1; ; ++attempts) {
    if (!state_.exchange(true, std::memory_order_acquire)) return;
    while (state_.load(std::memory_order_relaxed)) {
      std::this_thread::yield();
    }
    if (attempts % 16 == 0) std::this_thread::sleep_for(std::chrono::nanoseconds(attempts));
  }
}

void SpinLock::unlock() { state_.store(false, std::memory_order_release); }

bool SpinLock::try_lock() { return !state_.exchange(true, std::memory_order_acquire); }
```

**How the tests verify you:** `CounterTest` runs `kThreads` spinning `lock()`
increments of a shared counter and must observe exactly `kThreads * kIncrements`;
`TryLockTest` must contend and win ≥1; `SingleThreadReenters` expects `try_lock()`
to release properly. The all-green bar requires the backoff, not just a bare
`exchange` loop.