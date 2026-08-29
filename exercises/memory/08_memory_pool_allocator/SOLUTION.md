# Exercise 08 — Memory Pool Allocator (Reference Solution)

**What you implement:** a fixed-capacity object pool of `T` blocks whose free
list is embedded in the slots themselves (intrusive singly-linked list), so
`deallocate` → `allocate` reuses storage in constant time without any
per-object bookkeeping.

**Approach**
- The pool `::operator new[]`s one contiguous byte arena, then threads *slots*
  onto an intrusive free list by writing `reinterpret_cast<Node*>(slot)->next`,
  and pushes the head in `slot_bytes` steps through the arena.
- `allocate()` pops the free-list head (LIFO — constant time, cache-friendly)
  and `placement new`s `T` onto the slot; `nullptr` when exhausted.
- `deallocate(p)` runs `p->~T()` then pushes the slot back onto the list.
- The slot index ↔ pointer arithmetic is the classic "next-free stored inside
  the object" trick: deallocated memory may be reused freely, so each slot's
  first word can double as the `next` link.
- Alignment: `slot_bytes = sizeof(T)` rounded up to alignof(T); the arena
  base (fresh `operator new[]`) is already sufficiently aligned.

## Reference API — `include/pool_allocator.h`
#ifndef EXERCISE08_POOL_ALLOCATOR_H_
#define EXERCISE08_POOL_ALLOCATOR_H_

#include <cstddef>
#include <new>
#include <utility>

// Fixed-capacity, object-pool allocator: a preallocated arena of `capacity`
// slots handed out by freelist. O(1) alloc/dealloc, no syscalls in the hot
// path.
//
// Contract:
//   - `allocate()` returns a pointer to a live, placement-new-constructed `T`
//     (its constructor HAS run). Returns `nullptr` when the pool is exhausted.
//   - `deallocate(T*)` runs `~T()` and returns the slot to the freelist. Safe
//     to call on `nullptr` (no-op).
//   - Pool exhaustion + deallocation reuses the SAME slot.
//   - NOT thread-safe in the base version.
//
// Implementation: a raw byte arena sized in units of aligned T storage.
// Each free slot stores the index of the next free slot (intrusive freelist)
// in the first bytes of its storage. On allocate we placement-new a T; on
// deallocate we run ~T and re-link the slot onto the freelist. The destructor
// destroys any still-live occupants (slots not on the freelist), then frees
// the arena.

template <typename T>
class PoolAllocator {
 public:
  using value_type = T;

  explicit PoolAllocator(std::size_t capacity)
      : capacity_(capacity),
        align_(alignof(T) > alignof(std::max_align_t) ? alignof(T)
                                                      : alignof(std::max_align_t)) {
    const std::size_t a = align_;
    slot_bytes_ = (sizeof(T) > sizeof(void*) ? (sizeof(T) + a - 1) / a * a : sizeof(void*));
    arena_ = ::operator new(capacity_ * slot_bytes_, std::align_val_t(a));

    // Build the intrusive freelist: slot i's link word points to i+1; the last
    // slot points to kSentinel (empty).
    for (std::size_t i = 0; i < capacity_; ++i) {
      set_link(i, (i + 1 < capacity_) ? i + 1 : kSentinel);
    }
  }

  ~PoolAllocator() {
    if (arena_ == nullptr) return;
    // Mark free slots; the rest still hold live T objects we must destroy.
    unsigned char* marked = new unsigned char[capacity_];
    for (std::size_t i = 0; i < capacity_; ++i) marked[i] = 0;
    for (std::size_t idx = head_; idx != kSentinel; idx = get_link(idx)) marked[idx] = 1;
    for (std::size_t i = 0; i < capacity_; ++i) {
      if (!marked[i]) slot_ptr(i)->~T();
    }
    delete[] marked;
    ::operator delete(arena_, std::align_val_t(align_));
  }

  PoolAllocator(const PoolAllocator&) = delete;
  PoolAllocator& operator=(const PoolAllocator&) = delete;

  T* allocate() {
    if (head_ == kSentinel) return nullptr;
    const std::size_t idx = head_;
    head_ = get_link(idx);
    return ::new (static_cast<void*>(slot_ptr(idx))) T();
  }

  void deallocate(T* p) {
    if (p == nullptr) return;
    p->~T();
    const unsigned char* base = static_cast<unsigned char*>(arena_);
    const std::size_t idx =
        static_cast<std::size_t>(reinterpret_cast<const unsigned char*>(p) - base) / slot_bytes_;
    set_link(idx, head_);
    head_ = idx;
  }

  std::size_t capacity() const { return capacity_; }

 private:
  static constexpr std::size_t kSentinel = ~std::size_t{0};

  void set_link(std::size_t idx, std::size_t next) {
    link_of(idx) = next;
  }
  std::size_t get_link(std::size_t idx) const { return link_of(idx); }

  std::size_t& link_of(std::size_t idx) const {
    return *reinterpret_cast<std::size_t*>(static_cast<unsigned char*>(arena_) + idx * slot_bytes_);
  }

  T* slot_ptr(std::size_t idx) const {
    return reinterpret_cast<T*>(static_cast<unsigned char*>(arena_) + idx * slot_bytes_);
  }

  void* arena_{nullptr};
  std::size_t capacity_{0};
  std::size_t slot_bytes_{0};
  std::size_t head_{0};
  std::size_t align_{0};
};

#endif  // EXERCISE08_POOL_ALLOCATOR_H_

## Reference instantiation — `src/pool_allocator.cpp`
#include "pool_allocator.h"

#include <cstdint>
#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers this
// title). Keeping this TU pins the instantiations exercised by the unit tests.

template class PoolAllocator<std::uint64_t>;
template class PoolAllocator<std::string>;