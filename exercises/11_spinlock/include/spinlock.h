#ifndef EXERCISE11_SPINLOCK_H_
#define EXERCISE11_SPINLOCK_H_

#include <atomic>

// A test-and-test-and-set (TTAS) spinlock with exponential backoff.
//
// Contract (a C++ `BasicLockable`/`Lockable`, so it works with
// std::lock_guard / std::unique_lock / std::scoped_lock):
//   lock()      - blocks until acquired. No fairness required.
//   unlock()    - releases; must be called by the holder. Releasing without
//                 holding is UB (like std::mutex).
//   try_lock()  - true iff acquired without blocking.
//
// Not recursive.
//
// TODO(anwer): implement lock()/unlock()/try_lock() in src/spinlock.cpp.
//   - test-and-test-and-set on `state_` to avoid cache-line ping-pong:
//       while (state_.exchange(true)) { while (state_.load()) /* backoff */; }
//     or an explicit TAS on a copy of the load when unlocked (TTAS).
//   - exponential backoff on contention: start with std::this_thread::yield(),
//     then bounded std::this_thread::sleep_for, doubling up to a cap
//     (e.g. ~512 us), so the cache line gets a chance to be released.
//   - use std::atomic<bool> (or std::atomic_flag) with seq_cst (fine at this
//     scale; you can tighten to acquire/release if you want to defend it).

class SpinLock {
 public:
  SpinLock() noexcept = default;

  SpinLock(const SpinLock&) = delete;
  SpinLock& operator=(const SpinLock&) = delete;

  void lock();
  void unlock();
  bool try_lock();

 private:
  std::atomic<bool> state_{false};
};

#endif  // EXERCISE11_SPINLOCK_H_