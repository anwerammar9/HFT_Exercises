# Exercise 08 — Memory Pool Allocator (Task)

## Problem
A fixed-capacity pool of `T` blocks whose free list is embedded **through the
slots themselves** (intrusive singly-linked list), so deallocate→allocate
reuses storage in constant time with zero per-object bookkeeping.

## Requirements (what the tests check)
1. `allocate()` returns a pointer to a **placement-new-constructed** `T` (the
   constructor HAS run). Returns `nullptr` when exhausted — no throw.
2. `deallocate(T*)` runs `~T()` and returns the slot to the free list; safe on
   a `nullptr` argument.
3. Exhaustion + deallocate reuses the **same** slot (pointer-identity test —
   proves real O(1) reuse, not leaking).
4. Allocated pointers are correctly sized/aligned for `T`.
5. Ctor/dtor counts balance exactly with allocate/deallocate calls (tracked
   test type).
6. Single-threaded is a valid, documented choice; make it thread-safe only if
   you want the concurrent stress test enabled.

## Public API
```cpp
template <typename T>
class PoolAllocator {
  using value_type = T;
  explicit PoolAllocator(std::size_t capacity);
  ~PoolAllocator();            // destroy live T's, free the arena
  T* allocate();               // pop freelist, placement-new T
  void deallocate(T* p);       // ~T, push slot
  std::size_t capacity() const;
};
```

## Design notes
Byte arena of `capacity · slot_bytes` with
`slot_bytes = max(sizeof(T), sizeof(void*))` rounded up to
`max(alignof(max_align_t), alignof(T))`. Slot i's first word = index of the
next free slot (`kSentinel` = tail).

## Files
- Stub: `include/pool_allocator.h` (template — implement inline)
- Tests: `test/test_pool_allocator.cpp`
- Reference: `SOLUTION.md`