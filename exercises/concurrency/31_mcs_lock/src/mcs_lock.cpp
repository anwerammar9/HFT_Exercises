#include "mcs_lock.h"

// TODO(anwer): implement lock/unlock (see SOLUTION.md).
//
//   lock(w):
//     w.next_.store(nullptr, relaxed);
//     w.locked_.store(true, relaxed);
//     prev = tail_.exchange(&w, acq_rel);
//     if (prev != nullptr) {
//       prev->next_.store(&w, release);
//       while (w.locked_.load(acquire)) std::this_thread::yield();
//     }
//
//   unlock(w):
//     if (w.next_.load(relaxed) == nullptr) {
//       if (tail_.compare_exchange_weak(w, nullptr, release, relaxed)) return;
//       while (w.next_.load(relaxed) == nullptr) std::this_thread::yield();
//     }
//     w.next_.load(acquire)->locked_.store(false, release);

void McsLock::lock(Waiter& /*w*/) {}

void McsLock::unlock(Waiter& /*w*/) {}