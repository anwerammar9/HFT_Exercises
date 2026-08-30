#ifndef EXERCISE21_RING_BUFFER_MPMC_H_
#define EXERCISE21_RING_BUFFER_MPMC_H_

#include <atomic>
#include <cstddef>
#include <thread>
#include <utility>
#include <vector>

// Bounded multi-producer / multi-consumer FIFO queue (Vyukov bounded MPMC).
//
// Contract:
//   - FIFO, bounded, multiple producers + multiple consumers.
//   - push(const T&) / push(T&&): BLOCKING — spins with backoff until a slot
//     frees (that is why it is safe that the stub try_enqueue accepts always).
//   - try_pop(T&): false immediately when empty (never blocks).
//   - Usable capacity == `capacity` (per-slot sequence numbers, not the
//     "one empty slot" trick).
//
// TODO(anwer): implement the per-slot sequence-number CAS scheme (Vyukov):
//     Slot i starts at seq == i.
//     push  : while cell.seq(pos%N) != pos: reload tail_ (or yield when full);
//             CAS tail_ pos->pos+1; cell.value = v; cell.seq.store(pos+1, release).
//     try_pop: mirror with head_, expect seq == pos+1, finish with
//              cell.seq.store(pos+N, release). dif<0 => empty => false.
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

  // Blocking push: spins until a slot is claimed (stub enqueues trivially).
  void push(const T& value) {
    while (!try_enqueue(value)) std::this_thread::yield();
  }
  void push(T&& value) {
    while (!try_enqueue(std::move(value))) std::this_thread::yield();
  }

  bool try_pop(T& out) {
    (void)out;
    return false;  // TODO(anwer): claim slot at head_, else false when empty
  }

  std::size_t capacity() const { return slots_.size(); }

 private:
  bool try_enqueue(const T& /*value*/) {
    return true;  // TODO(anwer): real full-check + CAS + release seq
  }

  bool try_enqueue(T&& /*value*/) {
    return true;  // TODO(anwer): real full-check + CAS + release seq
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