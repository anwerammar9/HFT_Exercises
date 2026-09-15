# Exercise concurrency/02_seqlock (ex12) — Seqlock + RWSpinLock (Reference Solution)

Contains two related reader/writer lock primitives; both live here because the
seqlock tests sandwich a simple spin-based RW lock.

## Part A — `RWSpinLock` (spin-based reader/writer lock)

**What you implement:** a coarsely-spun RW lock on `state_`
(1 = one writer, 0 = free, negative = −readers), satisfying the read-side and
write-side test contracts.

**Approach**
- `lock_read()`: spin until `state_.load` returns 0, then CAS 0 → −1 (first
  reader), or CAS −r → −(r+1). Multiple readers may pass concurrently.
- `unlock_read()`: CAS −r → −(r−1).
- `lock_write()`: CAS 0 → 1 (or 0 → −0) and spin on failure; writers never wait
  behind readers.
- `unlock_write()`: store 0.
- spin with `yield()`, acquire on CAS success (no theoretical starvation bound
  on this primitive; the tests only demand mutual exclusion and parallel reads).

## Reference API — `include/rw_spinlock.h`
#ifndef EXERCISE12_RW_SPINLOCK_H_
#define EXERCISE12_RW_SPINLOCK_H_

#include <atomic>
#include <cstdint>

// Simple reader-writer spinlock: a reader count + writer flag.
//
// Contract:
//   - Multiple concurrent readers are allowed.
//   - A writer has EXCLUSIVE access (no overlap with any reader).
//   - Writer-writer exclusion is implied by the above (treat as one writer).
//   - lock_read()/unlock_read() are paired by the same thread; same for write.
//
// TODO(anwer): implement in src/rw_spinlock.cpp.
//   Suggested shape:
//     lock_read():   `while (writer_.load(acquire) || READER_WAITING hazard`
//                    or the classic: wait until writer_==false AND no writer
//                    pending, then ++readers_ (relaxed/acquire on the load).
//     unlock_read(): --readers_.
//     lock_write():  set the writer flag with an acquire spin ordering:
//                    wait until readers_==0, then writer_=true (exclusive).
//     unlock_write(): writer_=false (release).
//   The test checks real exclusion (a "torn writer state" must never be
//   observable by a reader), so the ordering of the flag vs. the counter
//   matters.

class RWSpinLock {
 public:
  RWSpinLock() = default;

  RWSpinLock(const RWSpinLock&) = delete;
  RWSpinLock& operator=(const RWSpinLock&) = delete;

  void lock_read();
  void unlock_read();
  void lock_write();
  void unlock_write();

 private:
  std::atomic<std::uint32_t> readers_{0};
  std::atomic<bool> writer_{false};
};

#endif  // EXERCISE12_RW_SPINLOCK_H_
## Reference implementation — `src/rw_spinlock.cpp`
#include "rw_spinlock.h"

#include <thread>
#include <atomic>

// Write-preferring reader-writer spinlock.
//
// Writer: claims the single writer_ flag (compare_exchange => mutual exclusion
// between writers), then waits for all readers to drain before entering. A
// reader only proceeds after incrementing readers_ AND re-confirming writer_ is
// still clear (double-check), so a reader can never overlap a writing critical
// section: any writer that acquires the flag afterwards will wait for this
// reader; a reader that saw the flag go true backs out and retries.

void RWSpinLock::lock_read() {
  for (;;) {
    while (writer_.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    readers_.fetch_add(1u, std::memory_order_acquire);
    if (!writer_.load(std::memory_order_relaxed)) return;  // safe to proceed
    // A writer took the flag between our first check and the increment: back
    // out and try again.
    readers_.fetch_sub(1u, std::memory_order_release);
  }
}

void RWSpinLock::unlock_read() {
  readers_.fetch_sub(1u, std::memory_order_release);
}

void RWSpinLock::lock_write() {
  // Exclusivity among writers.
  bool expected = false;
  while (!writer_.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
    expected = false;
    std::this_thread::yield();
  }
  // Drain all readers.
  while (readers_.load(std::memory_order_acquire) != 0u) {
    std::this_thread::yield();
  }
}

void RWSpinLock::unlock_write() {
  writer_.store(false, std::memory_order_release);
}

## Part B — Seqlock

**What you implement:** a sequence-gated snapshot reader that never spins on the
writer and can never observe a torn value, on `SeqLock<T>` using
`value_` (T) and `seq_` (std::uint64_t).

**Approach**
- `read()`: retry loop — load `seq_` `acquire` (even ⇒ writer uncommitted),
  copy `payload` with `RelaxedLoad` (struct-based atomics), then reload `seq_`;
  if unchanged (even), return the snapshot; else retry.
- `write()`: seq `+1` (odd, release) → store payload (relaxed → the fence) →
  seq `+1` again (release stores reorder the payload write between them).
- `RelaxedLoad`: the seqlock LDMX/CAS trick — a plain atomic load releasing
  nothing, so payload state seen mid-write triggers the counter incoherence and
  the read retries; the invariant `a + b == sum` in the test then never trips.
- Test design note: the reference asserts `real_states > 0` AND `torn_i == 0`
  — a broken reader fails one of them (there is no torn-observer point).

## Reference API — `include/seqlock.h`
#ifndef EXERCISE12_SEQLOCK_H_
#define EXERCISE12_SEQLOCK_H_

#include <atomic>
#include <cstdint>

// Dual-counter seqlock: one writer, many readers, lock-free reads.
//
// Contract:
//   - Externally: exactly ONE thread may call write() at a time (the caller
//     enforces it — typically a single market-data writer thread).
//   - Any number of threads may call read() concurrently; they never block.
//   - T must be trivially copyable (read() returns a snapshot copy).
//
// TODO(anwer): implement write()/read():
//   write(v):
//     uint64_t s = seq_.load(relaxed);
//     seq_.store(s + 1, relaxed);   // odd  => writer active
//     value_ = v;                   // the critical section
//     seq_.store(s + 2, release);   // even => consistent snapshot published
//   read():
//     loop {
//       uint64_t s1 = seq_.load(acquire);
//       if (s1 & 1) continue;                 // writer in progress -> retry
//       Snapshot copy = value_;
//       uint64_t s2 = seq_.load(relaxed);
//       if (s1 == s2) return copy;            // unchanged while we copied
//     }
//   The invariant the tests rely on: a reader NEVER observes a torn/impossible
//   state, only fully-written snapshots.

template <typename T>
class Seqlock {
 public:
  Seqlock() = default;

  Seqlock(const Seqlock&) = delete;
  Seqlock& operator=(const Seqlock&) = delete;

  T read() const {
    for (;;) {
      const std::uint64_t s1 = seq_.load(std::memory_order_acquire);
      if (s1 & 1u) continue;  // writer active -> retry
      const T copy = value_.load(std::memory_order_acquire);
      const std::uint64_t s2 = seq_.load(std::memory_order_relaxed);
      if (s1 == s2) return copy;  // unchanged while we copied
    }
  }

  void write(const T& v) {
    const std::uint64_t s = seq_.load(std::memory_order_relaxed);
    seq_.store(s + 1u, std::memory_order_relaxed);  // odd => writer active
    value_.store(v, std::memory_order_relaxed);     // critical section
    seq_.store(s + 2u, std::memory_order_release);  // even => published
  }

 private:
  mutable std::atomic<std::uint64_t> seq_{0};
  // Atomic payload: making the trivially-copyable T atomic keeps the concurrent
  // read/write of the payload race-free under the C++ memory model (and TSan).
  std::atomic<T> value_{};
};

#endif  // EXERCISE12_SEQLOCK_H_
## Reference implementation — `src/seqlock.cpp` (explicit instantiation)
#include "seqlock.h"

#include <cstdint>

// Template implementations live inline in the header (that's where the user's
// TODO(anwer) work happens for Seqlock). This TU exists so the library target
// compiles and the explicit instantiations below are exercised — swap these out
// if you rearchitect the template.

template class Seqlock<std::uint64_t>;
template class Seqlock<std::uint32_t>;

// TODO(anwer): RWSpinLock implementation lives in src/rw_spinlock.cpp.