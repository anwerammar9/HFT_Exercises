#include "spinlock.h"

#include <thread>

// TODO(anwer): implement a TTAS spinlock with exponential backoff on `state_`.
//
// Suggested shape (see the reference solution in SOLUTION.md):
//   lock():
//     // test-and-test-and-set with exponential backoff
//     size_t delay = 1;
//     while (true) {
//       if (!state_.exchange(true, acquire)) return;   // fast-path acquire
//       while (state_.load(relaxed)) {                 // spin read-only so the
//         std::this_thread::yield();                   // cache line stays shared
//         for (size_t i = 0; i < delay; ++i) std::this_thread::yield();
//         if (delay < kMaxDelay) delay *= 2;
//       }
//     }
//   unlock(): state_.store(false, release);
//   try_lock(): !state_.exchange(true, acquire)

// These stubs are intentionally no-ops so the library builds and links and the
// tests run RED until the real logic lands.
void SpinLock::lock() {}

void SpinLock::unlock() {}

bool SpinLock::try_lock() { return false; }
