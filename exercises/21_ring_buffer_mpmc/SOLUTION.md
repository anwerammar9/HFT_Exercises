# Exercise 21 — Bounded MPMC Queue (Vyukov) (Reference Solution)

**What you implement:** a bounded multi-producer/multi-consumer FIFO with no
global lock. Each slot carries its own sequence number, so producers and
consumers claim slots with a CAS and never contend for a queue-wide lock.

**Approach**
- `Slot { T value; alignas(64) std::atomic<size_t> seq; }`. Slot `i` starts with
  `seq == i`.
- **push**: read `tail_` relaxed; the slot at `pos % size` has `seq == pos` iff
  it is free for THIS pass. CAS `tail_ pos→pos+1` claims it, release-store the
  value, then release-store `seq = pos + 1` (published for the consumer).
  `dif = seq - pos`: `0` ⇒ claimable; `< 0` ⇒ slot not yet recycled (FULL —
  spin with backoff until `dif >= 0`); `> 0` ⇒ stale pos, reload `tail_`.
- **try_pop**: mirror with `head_`, expecting `seq == pos + 1`; on success
  release-store `seq = pos + size` so the same slot can house the next
  generation. `< 0` ⇒ EMPTY → return false immediately (never blocks).
- **push stays blocking** (per the plan's `void push(T)` contract): spins +
  `yield` until a slot frees, with backoff. try_pop is the non-blocking read.
- Cache geometry: `alignas(64)` slots avoid producer/consumer false sharing.

## Reference API — `include/ring_buffer_mpmc.h`
#ifndef EXERCISE21_RING_BUFFER_MPMC_H_
#define EXERCISE21_RING_BUFFER_MPMC_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <utility>
#include <vector>

// Bounded multi-producer / multi-consumer FIFO queue (Vyukov bounded MPMC).
//
// Contract:
//   - FIFO, bounded, multiple producers + multiple consumers.
//   - push(const T&) / push(T&&): BLOCKING. When the queue is full the caller
//     spins (with backoff) until a slot frees. The plan's `void push(T)` is
//     why this is a blocking design; document the tradeoff if you change it.
//   - try_pop(T&): false immediately when empty (never blocks).
//   - Usable capacity == `capacity` (Vyukov's scheme doesn't need the "one
//     empty slot" trick; each slot carries its own sequence number).
//
// TODO(anwer): implement using per-slot sequence numbers so first-touch CAS
//   slot claiming works without a global lock. This is the classic Vyukov
//   bounded MPMC queue:
//     buffer_[i] holds {T value; std::atomic<uint32_t> seq}.
//     - all slots start with seq == i.
//     - push spins until `slot->seq == tail_` (relaxed/acquire CAS), then
//       writes the value (release), increments slot->seq to tail_+1, and bumps
//       tail_ (the "1,000,000" answer: next round the slot seq wraps to 0).
//     - try_pop does the mirror image with `head_` and `slot->seq == head_+1`.
//   Spin with backoff rather than sleep; you may add std::array<alignas(64)
//   slot, N> so producers/consumers on different slots don't share lines.

template <typename T>
class MpmcQueue {
 public:
  explicit MpmcQueue(std::size_t capacity) : slots_(capacity) {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
      slots_[i].seq.store(i, std::memory_order_relaxed);
    }
  }

  MpmcQueue(const MpmcQueue&) = delete;
  MpmcQueue& operator=(const MpmcQueue&) = delete;

  // Blocking push: spins (with backoff) until a free slot is claimed.
  void push(const T& value) {
    while (!try_enqueue(value)) std::this_thread::yield();
  }
  void push(T&& value) {
    while (!try_enqueue(std::move(value))) std::this_thread::yield();
  }

  bool try_pop(T& out) {
    std::size_t pos = head_.load(std::memory_order_relaxed);
    for (;;) {
      Slot& cell = slots_[pos % slots_.size()];
      const std::size_t seq = cell.seq.load(std::memory_order_acquire);
      const std::intptr_t dif =
          static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
      if (dif == 0) {
        if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
        // else: pos reloaded by compare_exchange_weak; retry.
      } else if (dif < 0) {
        return false;  // empty
      } else {
        pos = head_.load(std::memory_order_relaxed);
      }
    }
    Slot& cell = slots_[pos % slots_.size()];
    out = cell.value;
    cell.seq.store(pos + slots_.size(), std::memory_order_release);
    return true;
  }

  std::size_t capacity() const { return slots_.size(); }

 private:
  bool try_enqueue(const T& value) {
    std::size_t pos = tail_.load(std::memory_order_relaxed);
    for (;;) {
      Slot& cell = slots_[pos % slots_.size()];
      const std::size_t seq = cell.seq.load(std::memory_order_acquire);
      const std::intptr_t dif =
          static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
      if (dif == 0) {
        if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          cell.value = value;
          cell.seq.store(pos + 1, std::memory_order_release);
          return true;
        }
        // else: pos reloaded by compare_exchange_weak; retry.
      } else if (dif < 0) {
        return false;  // full
      } else {
        pos = tail_.load(std::memory_order_relaxed);
      }
    }
  }

  bool try_enqueue(T&& value) {
    std::size_t pos = tail_.load(std::memory_order_relaxed);
    for (;;) {
      Slot& cell = slots_[pos % slots_.size()];
      const std::size_t seq = cell.seq.load(std::memory_order_acquire);
      const std::intptr_t dif =
          static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
      if (dif == 0) {
        if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          cell.value = std::move(value);
          cell.seq.store(pos + 1, std::memory_order_release);
          return true;
        }
      } else if (dif < 0) {
        return false;  // full
      } else {
        pos = tail_.load(std::memory_order_relaxed);
      }
    }
  }

  struct Slot {
    T value{};
    alignas(64) std::atomic<std::size_t> seq{0};
  };
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
  std::vector<Slot> slots_;
};

#endif  // EXERCISE21_RING_BUFFER_MPMC_H_
## Reference TU — `src/ring_buffer_mpmc.cpp`
#include "ring_buffer_mpmc.h"

#include <cstdint>

// Implementation is inline in the header (see the TODO(anwer) markers).
// Keeping this TU pins the instantiations exercised by the unit tests.

template class MpmcQueue<int>;
template class MpmcQueue<std::uint64_t>;