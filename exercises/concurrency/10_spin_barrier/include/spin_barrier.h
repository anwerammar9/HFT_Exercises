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