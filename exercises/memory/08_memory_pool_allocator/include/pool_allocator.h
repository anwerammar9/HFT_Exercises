#ifndef EXERCISE08_POOL_ALLOCATOR_H_
#define EXERCISE08_POOL_ALLOCATOR_H_

#include <cstddef>
#include <new>

// TODO(anwer): implement a fixed-capacity object pool (see SOLUTION.md).
//
// Contract:
//   - `allocate()` returns a pointer to a placement-new-constructed `T`
//     (constructor HAS run). Returns `nullptr` when exhausted.
//   - `deallocate(T*)` runs `~T()` and returns the slot to the freelist; safe
//     on `nullptr`.
//   - Exhaustion + deallocate reuses the SAME slot.
//
// Suggested shape (see SOLUTION.md):
//   - Allocate a byte arena of `capacity_ * slot_bytes_` where
//     `slot_bytes_ = max(sizeof(T), sizeof(void*))` rounded up to
//     `alignof(std::max_align_t)` (or alignof(T) if greater).
//   - Thread an intrusive freelist THROUGH the slots: slot i's first word
//     holds the index of the next free slot (kSentinel for the tail).
//   - allocate(): pop head, placement-new T; deallocate(): ~T, push slot.
template <typename T>
class PoolAllocator {
 public:
  using value_type = T;

  explicit PoolAllocator(std::size_t capacity) : capacity_(capacity) {}

  ~PoolAllocator() {}  // TODO(anwer): destroy live T's, free the arena

  PoolAllocator(const PoolAllocator&) = delete;
  PoolAllocator& operator=(const PoolAllocator&) = delete;

  T* allocate();  // TODO(anwer): pop freelist, placement-new T

  void deallocate(T* p);  // TODO(anwer): ~T, push slot

  std::size_t capacity() const { return capacity_; }

 private:
  void* arena_{nullptr};
  std::size_t capacity_{0};
};

// Stub bodies (red until implemented).
template <typename T>
T* PoolAllocator<T>::allocate() {
  return nullptr;
}

template <typename T>
void PoolAllocator<T>::deallocate(T* /*p*/) {}

#endif  // EXERCISE08_POOL_ALLOCATOR_H_