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
//   - Bounded, fixed capacity N (a power of two; `idx & (N-1)` replaces `%`).
//   - USABLE capacity is N-1: one slot stays empty so `head == tail` can mean
//     EMPTY unambiguously.
//   - `head`/`tail` are monotonically increasing counters, never wrapped —
//     that's what makes this lock-free.
//   - Exactly one producer thread and one consumer thread. Otherwise UB.
//
// TODO(anwer): implement the five methods (see SOLUTION.md).
template <typename T, std::size_t N>
class SpscRingBuffer {
 public:
  static_assert((N & (N - 1)) == 0, "N must be a power of two");
  static constexpr std::size_t kCapacity = N - 1;  // usable slots

  SpscRingBuffer() = default;

  SpscRingBuffer(const SpscRingBuffer&) = delete;
  SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

  bool try_push(T value) {
    (void)value;
    return true;  // TODO(anwer): full-check + move into slot + release tail
  }

  bool try_pop(T& out) {
    out = T{};  // TODO(anwer): empty-check + read slot + release head
    return true;
  }

  std::size_t size() const { return 0; }  // TODO(anwer): tail - head

  bool empty() const { return true; }  // TODO(anwer): tail == head

  bool full() const { return false; }  // TODO(anwer): tail - head == kCapacity

 private:
  alignas(64) std::atomic<std::uint64_t> head_{0};  // consumer
  alignas(64) std::atomic<std::uint64_t> tail_{0};  // producer
  std::array<T, N> buffer_{};
};

#endif  // EXERCISE13_RING_BUFFER_SPSC_H_