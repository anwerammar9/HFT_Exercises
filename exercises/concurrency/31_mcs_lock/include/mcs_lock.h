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