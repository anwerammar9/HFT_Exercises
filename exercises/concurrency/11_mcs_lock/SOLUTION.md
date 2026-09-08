# Exercise concurrency/11_mcs_lock (ex29) — MCS Queue Lock (Reference Solution)

**What you implement:** a fair MCS queue lock where each contender spins on its
own flag and the holder hands the lock to the next queued thread, giving
O(1) per-thread contention plus FIFO hand-off order.

**Approach**
- State is a singly-linked implicit queue: `tail_` points to the last enqueued
  `Waiter`; each `Waiter`'s `next_` points to the one enqueued behind it.
- `lock(w)`:
  1. `w.next_ = nullptr; w.locked_ = true;` (both relaxed — private to w until
     it's published).
  2. `prev = tail_.exchange(&w, acq_rel)` — enqueue. `prev == nullptr`? We won
     the lock immediately.
  3. Else link behind `prev`: `prev->next_ = &w` (release — publishes the whole
     critical section w will do), then spin on **own** `w.locked_` (acquire),
     yielding between polls.
- `unlock(w)`:
  1. If `w.next_ == nullptr` (relaxed): try to empty the queue — CAS `tail_`
     from `&w` to `nullptr`. Win → done, no successor to wake. Lose → a new
     successor linked in *just now*; they may not have finished setting
     `w.next_`, so yield-spin until it appears.
  2. Otherwise `w.next_.load(acquire)->locked_.store(false, release)`.
- Why it works: the acquire spin on my own flag pairs with my predecessor's
  release store of `next_` and their release store clearing my `locked_` — a
  release sequence that hands the critical section to exactly one successor.

## Reference API — `include/mcs_lock.h`

```cpp
#ifndef EXERCISE29_MCS_LOCK_H_
#define EXERCISE29_MCS_LOCK_H_

#include <atomic>

// MCS queue-based mutual-exclusion lock (Mellor-Crummey & Scott, 1991).
//
// A queue lock is the FAIR answer to a spinlock: instead of every waiter
// slamming a single shared atomic (the spinlock's cache-line ping-pong), each
// waiter spins on ITS OWN flag, and the holder hands the lock to the NEXT
// thread in line. Outcome: O(1) contention per thread, no worst-case
// starvation, and deterministic FIFO hand-offs — the deal-breaker for a
// super-busy exchange lock or a cross-socket sharded counter.
//
// The price is the API: the per-thread state lives in the caller's Waiter
// node, not inside the lock, so you must hand the lock a node on every call:
//
//   McsLock lock;
//   McsLock::Waiter node;            // one per lock-using thread
//   lock.lock(node);                 // enqueue + spin until it's our turn
//   ... critical section ...
//   lock.unlock(node);               // node MUST be the current holder
//
// Contract:
//   - A Waiter is used by ONE thread at a time and must outlive the
//     lock()/unlock() pair it participates in. unlock() must receive the same
//     node the thread lock()ed with.
//   - Waiter may be REUSED across many lock/unlock rounds — that is the normal
//     pattern (the node is just "my flag + my next pointer").
//   - FIFO acquire order (fairness); not recursive.
//
// TODO(anwer): implement lock/unlock in src/mcs_lock.cpp.
//
//   lock(w):                             // enqueue w at the tail
//     w.next_.store(nullptr, relaxed);   // I am (for now) the last waiter
//     w.locked_.store(true, relaxed);    // my flag: armed
//     prev = tail_.exchange(&w, acq_rel);
//     if (prev != nullptr) {             // somebody was already queued
//       prev->next_.store(&w, release);  //   link behind them (they'll spin us)
//       while (w.locked_.load(acquire)) std::this_thread::yield();  // wait my turn
//     }
//     // (prev == nullptr: I acquired immediately, tail_ already points at me)
//
//   unlock(w):                           // hand off to whoever is behind me
//     if (w.next_.load(relaxed) == nullptr) {
//       // I might be the last waiter. Try to shrink the queue to empty:
//       if (tail_.compare_exchange_weak(w, nullptr, release, relaxed)) return;
//       // CAS lost -> a new successor JUST linked behind us; wait for them to
//       // finish storing their address in w.next_ ...
//       while (w.next_.load(relaxed) == nullptr) std::this_thread::yield();
//     }
//     w.next_.load(acquire)->locked_.store(false, release);  // release successor

class McsLock {
 public:
  class Waiter {
   public:
    Waiter() = default;

   private:
    friend class McsLock;
    std::atomic<bool> locked_{true};
    std::atomic<Waiter*> next_{nullptr};
  };

  McsLock() = default;

  McsLock(const McsLock&) = delete;
  McsLock& operator=(const McsLock&) = delete;

  void lock(Waiter& w);
  void unlock(Waiter& w);

 private:
  std::atomic<Waiter*> tail_{nullptr};
};

#endif  // EXERCISE29_MCS_LOCK_H_
```

## Reference implementation — `src/mcs_lock.cpp`

```cpp
#include "mcs_lock.h"

#include <thread>

void McsLock::lock(Waiter& w) {
  w.next_.store(nullptr, std::memory_order_relaxed);
  w.locked_.store(true, std::memory_order_relaxed);

  Waiter* const prev = tail_.exchange(&w, std::memory_order_acq_rel);
  if (prev != nullptr) {
    prev->next_.store(&w, std::memory_order_release);
    while (w.locked_.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  }
}

void McsLock::unlock(Waiter& w) {
  if (w.next_.load(std::memory_order_relaxed) == nullptr) {
    Waiter* expected = &w;
    if (tail_.compare_exchange_weak(expected, nullptr, std::memory_order_release,
                                    std::memory_order_relaxed)) {
      return;  // queue emptied: nobody waiting behind me
    }
    // A successor just linked in between the null-check and the CAS above.
    // They saw us as the tail and are about to store their address in
    // w.next_; wait for that store to land before dereferencing.
    while (w.next_.load(std::memory_order_relaxed) == nullptr) {
      std::this_thread::yield();
    }
  }
  // Hand the lock to whoever queued behind us.
  w.next_.load(std::memory_order_acquire)
      ->locked_.store(false, std::memory_order_release);
}
```

**How the tests verify you:** `MutualExclusionCounter` and `OnlyOneHolderAtATime`
are the correctness discriminators (red against a no-op stub).
`AllThreadsMakeProgressUnderContention` bounds liveness. `SingleNodeReusedAcrossManyRounds`
and `TwoThreadsPingPongBothComplete` exercise node reuse and the head-of-queue
CAS hand-off dance. All green under `build-tsan` too.