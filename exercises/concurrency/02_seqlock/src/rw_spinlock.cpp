#include "rw_spinlock.h"

#include <thread>

// TODO(anwer): implement a spin-based reader/writer lock on `state_`
// (1 = writer, 0 = free, negative = -readers). See SOLUTION.md.
//
// Suggested shape:
//   lock_read():   spin while state_.load(relaxed) != 0, then
//                  CAS up/down the reader count (0 -> -1, -r -> -(r+1)).
//   unlock_read(): CAS -r -> -(r-1).
//   lock_write():  CAS 0 -> 1, spin on failure.
//   unlock_write(): store 0, release.

// These stubs are no-ops so the lib builds; the concurrent tests go RED.
void RWSpinLock::lock_read() {
    
}

void RWSpinLock::unlock_read() {}

void RWSpinLock::lock_write() {}

void RWSpinLock::unlock_write() {}