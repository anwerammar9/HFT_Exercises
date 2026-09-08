# Exercise concurrency/03_ring_buffer_spsc (ex13) — SPSC Lock-Free Ring Buffer (Reference Solution)

**What you implement:** a single-producer/single-consumer lock-free ring buffer
with one reserved "gap" slot so `head == tail` unambiguously marks EMPTY (usable
capacity is `N-1`). `head`/`tail` are monotonically increasing unbounded
counters, never wrapped, so full/empty tests are pure subtraction and the
`idx & (N-1)` masking handles the physical wrap on `buffer_`.

**Approach**
- `try_push`: read own `tail_` relaxed, peer `head_` acquire; full if
  `t - h >= kCapacity`. Move payload into `buffer_[t & mask]`, then
  `tail_.store(t+1, release)` — the release publishes the payload write.
- `try_pop`: read own `head_` relaxed, peer `tail_` acquire; the acquire
  orders the subsequent `buffer_[h & mask]` read after the producer's release
  (happens-before via the tail counter). `head_.store(h+1, release)` hands the
  slot back.
- `size/empty/full`: relaxed loads, difference on the two counters.
- Cache geometry: `head_`/`tail_` are `alignas(64)` in their own lines; the two
  threads only share `buffer_` slots, so no false sharing on the counters.
- Why it's lock-free: no primitives block or spin — each thread touches its own
  counter except one correctly-ordered peer read per operation.

## Reference API — `include/ring_buffer_spsc.h`
#ifndef EXERCISE13_RING_BUFFER_SPSC_H_
#define EXERCISE13_RING_BUFFER_SPSC_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

// Single-producer / single-consumer lock-free ring buffer over a fixed array.
//
// Contract:
//   - Bounded, fixed capacity N (must be a power of two so `idx & (N-1)`
//     replaces `idx % N`).
//   - USABLE capacity is N-1: one slot is always left empty so `head == tail`
//     unambiguously means empty (no separate counter needed).
//   - `head` and `tail` are monotonically increasing counters, never wrapped
//     (which is what makes this lock-free and safe in the long run).
//   - Exactly ONE producer thread and ONE consumer thread. Any other use is UB.
//
// TODO(anwer): implement the five methods below.
//   Memory ordering:
//     - producer writes its own tail (relaxed reads of head are fine for the
//       SPSC full-check, but `tail.store(tail_+1, release)` publishes the
//       payload write);
//     - consumer reads its own head relaxed and pops the slot at head, then
//       `head.fetch_add(1)` (the payload read must happen-before, so the slot
//       read + a relaxed head bump needs an acquire when READING the data slot;
//       the classic formulation is `data[p].acquire` via the atomic release of
//       the producer's tail — spell out the ordering in a comment and defend
//       it).
//   False sharing: put `head` and `tail` in their own cache lines
//   (`alignas(64)`); the payload array is thread-shared at the slots.
//
//   Suggested members (already declared): `head_` (consumer), `tail_`
//   (producer), `std::array<T, N> buffer_`. All public methods are currently
//   stubs so the tests build and run red.

template <typename T, std::size_t N>
class SpscRingBuffer {
 public:
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
  static constexpr std::size_t kCapacity = N - 1;  // usable slots

  SpscRingBuffer() = default;

  SpscRingBuffer(const SpscRingBuffer&) = delete;
  SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

  bool try_push(T value) {
    const std::uint64_t t = tail_.load(std::memory_order_relaxed);
    const std::uint64_t h = head_.load(std::memory_order_acquire);
    if (t - h >= kCapacity) return false;  // full
    buffer_[t & (N - 1)] = std::move(value);
    // Publish the payload: release on tail so the consumer's acquire on tail
    // happens-after this write.
    tail_.store(t + 1, std::memory_order_release);
    return true;
  }

  bool try_pop(T& out) {
    const std::uint64_t h = head_.load(std::memory_order_relaxed);
    const std::uint64_t t = tail_.load(std::memory_order_acquire);
    if (h == t) return false;  // empty (not allowed to touch out)
    out = buffer_[h & (N - 1)];
    // The acquire on `tail` above ordered our payload read after the producer's
    // release store; a relaxed bump is enough to let the producer reuse the slot.
    head_.store(h + 1, std::memory_order_release);
    return true;
  }

  std::size_t size() const {
    return static_cast<std::size_t>(tail_.load(std::memory_order_relaxed) -
                                    head_.load(std::memory_order_relaxed));
  }

  bool empty() const {
    return tail_.load(std::memory_order_relaxed) == head_.load(std::memory_order_relaxed);
  }

  bool full() const {
    return tail_.load(std::memory_order_relaxed) - head_.load(std::memory_order_relaxed) ==
           kCapacity;
  }

 private:
  alignas(64) std::atomic<std::uint64_t> head_{0};  // consumer
  alignas(64) std::atomic<std::uint64_t> tail_{0};  // producer
  std::array<T, N> buffer_{};
};

#endif  // EXERCISE13_RING_BUFFER_SPSC_H_
## Reference TU — `src/ring_buffer_spsc.cpp`
#include "ring_buffer_spsc.h"

#include <cstdint>

// The SpscRingBuffer implementation is inline in the header (see the
// TODO(anwer) markers there). This TU keeps the library target and pins the
// template instantiations used by the tests so a stub linker error can't hide
// an implementation gap.

template class SpscRingBuffer<std::uint64_t, 1024>;
template class SpscRingBuffer<std::uint32_t, 8>;