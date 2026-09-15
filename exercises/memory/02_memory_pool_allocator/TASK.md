# Exercise memory/02_memory_pool_allocator (ex08) — Memory Pool Allocator (Task)

## The problem (in plain words)

When a system allocates and frees objects of **one fixed size** (e.g. order
nodes, message envelopes), calling `malloc`/`free` per object wastes time and
fragments memory. A pool allocator pre-allocates a big block of `capacity`
slots and threads a **free list through the slots themselves** — the first word
of a free slot stores the index of the next free slot. `deallocate` → next
`allocate` of the same slot is then **O(1)** with zero per-object bookkeeping.

## Requirements (what the tests check)

1. `allocate()` returns a pointer to a **fully constructed** `T` — the
   constructor *has run* (via placement `new`). When the pool is exhausted it
   returns **`nullptr`** (no throw).
2. `deallocate(T*)` destroys the object (`~T()`) and pushes the slot back onto
   the free list; calling it on `nullptr` is safe.
3. Exhaust + deallocate then reuses the **same slot** — the tests assert pointer
   identity, which proves real O(1) reuse and not allocation-thrash.
4. Returned pointers are correctly sized and aligned for `T`.
5. Constructor/destructor counts balance **exactly** with allocate/deallocate
   calls (tracked test type).
6. Single-threaded is a valid, documented choice. Make it thread-safe **only**
   if you intend to enable the (initially disabled) concurrent stress test.

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

The stub lives in `include/pool_allocator.h` (a template) — implement the
methods inline, replacing the `TODO(anwer)` bodies.

## How to think about it (suggested design)

- Allocate one byte arena of `capacity · slot_bytes` where
  `slot_bytes = max(sizeof(T), sizeof(void*))` rounded up to
  `alignof(std::max_align_t)` (or `alignof(T)`, whichever is larger) — every
  slot must be big and aligned enough to hold both a `T` and a raw pointer.
- Slot *i*'s first `sizeof(void*)` bytes hold the **index of the next free
  slot** (`kSentinel` marks the tail). `head_` is the first free slot.
- `allocate()`: pop the head (reinterpreting those bytes), placement-new a `T`
  into the slot, return it. `deallocate()`: run `~T()`, then write the old head
  into the slot's first word and push it.
- When in doubt, follow the pointer-identity contract: allocate everything,
  deallocate one, allocate again — same address, constructor ran both times.

## Make it harder (optional — not covered by the tests)

- **Thread-safe pool:** protect `head_` with a `std::mutex` and enable the
  disabled concurrent test; then try the lock-free version on an
  `std::atomic` head (a mini version of Exercise 23's freelist).
- **Poisoning:** on `deallocate`, fill the slot with a fixed pattern (e.g.
  `0xDEADBEEF`) so use-after-free is trivially visible in a debugger.
- **Growth-on-demand:** a second constructor flag that, on exhaustion, chains a
  *new arena* instead of returning `nullptr` — compare against the fixed pool
  with a micro-benchmark.
- **Statistical reporting:** track `peak_live`, `allocs`, `frees`, and
  `reuse_hits` (a free→allocate that reused the most recently freed slot).

## Files

- Stub: `include/pool_allocator.h` (template — implement inline)
- Tests: `test/test_pool_allocator.cpp`
- Reference: `SOLUTION.md`