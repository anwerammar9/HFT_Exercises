#include "spin_barrier.h"

#include <stdexcept>

// TODO(anwer): implement wait() (see SOLUTION.md).
//
//   std::size_t round = generation_.load(std::memory_order_relaxed);
//   std::size_t arrived =
//       count_.fetch_add(1, std::memory_order_acq_rel) + 1;
//   if (arrived == n_) {
//     count_.store(0, std::memory_order_relaxed);
//     generation_.store(round + 1, std::memory_order_release);  // last arriver
//   } else {
//     while (generation_.load(std::memory_order_acquire) == round) {
//       std::this_thread::yield();
//     }
//   }

SpinBarrier::SpinBarrier(std::size_t n) : n_(n) {
  if (n == 0) throw std::invalid_argument("SpinBarrier size must be > 0");
}

void SpinBarrier::wait() {}

std::size_t SpinBarrier::generation() const noexcept { return 0; }