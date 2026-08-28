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