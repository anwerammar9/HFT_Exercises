#ifndef EXERCISE12_SEQLOCK_H_
#define EXERCISE12_SEQLOCK_H_

#include <atomic>
#include <cstdint>

#include "rw_spinlock.h"  // NOT used by the seqlock; kept for the include graph

// TODO(anwer): implement a seqlock snapshot reader on `value_` and `seq_`
// (see SOLUTION.md). The writer stores a payload + counter that is monotone;
// the reader must retry its snapshot whenever the counter changed mid-copy.
//
// Suggested shape:
//   read()  ->  loop { old = seq_.load(acquire); if (old & 1) continue;
//                     v = payload_; new = seq_.load(relaxed); if (new == old) return v; }
//   write() ->  seq_.store(seq_+1, release)   // odd: writer in critical section
//               payload_ = value;             // relaxed is fine here
//               seq_.store(seq_+1, release)   // even again: snapshot valid
template <typename T>
class Seqlock {
 public:
  Seqlock() = default;

  T read() const { return {}; }

  void write(const T& v) {
    (void)v;  // TODO(anwer): store payload
  }

 private:
  T value_{};
  mutable std::atomic<std::uint64_t> seq_{0};
};

#endif  // EXERCISE12_SEQLOCK_H_