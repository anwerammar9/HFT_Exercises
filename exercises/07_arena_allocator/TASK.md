# Exercise 07 — Arena Allocator (Task)

## Problem
A block-chained **bump** allocator: variable-size, alignment-correct blocks
handed out from big contiguous chunks; everything is reclaimed in bulk via
`reset()`/`release()`/`rollback_to()` — never per-object free. This is the HFT
"scratch memory" layer so per-message work never touches the system allocator.

## Requirements (what the tests check)
1. `allocate(size, alignment = alignof(max_align_t))`: storage of ≥ `size`
   bytes aligned to `alignment`; returned addresses **never overlap** across
   undispatched allocations; `allocate(0)` still returns a valid, unique,
   non-null pointer.
2. A request that does not fit the current chunk **chains a fresh chunk**.
3. `deallocate(p)` is a documented **NO-OP** (memory comes back in bulk); it
   must not corrupt state.
4. `reset()`: rewind to the start of the first chunk. Memory handed out before
   is reusable — the next allocation lands on the **same address** (no OS
   round-trip); `used_bytes()` → 0, `capacity()` unchanged, chunks retained.
5. `release()`: free every chunk; the arena is brand new
   (`capacity()==0`, `block_count()==0`) yet still usable.
6. `allocate_all()` returns a "mark" (commits everything so far);
   `rollback_to(mark)` undoes **only** allocations after the mark (strict stack
   discipline); marks can be nested/taken repeatedly.
7. `capacity()` / `used_bytes()` / `block_count()` report bytes owned, bytes
   handed out (live, summed across chunks), chunk count.

## Public API
```cpp
class ArenaAllocator {
  static constexpr std::size_t kDefaultBlockSize = 64 * 1024;
  explicit ArenaAllocator(std::size_t block_size = kDefaultBlockSize);
  ~ArenaAllocator();
  void* allocate(std::size_t size,
                 std::size_t alignment = alignof(std::max_align_t));
  void deallocate(void* ptr) noexcept;
  void* allocate_all();
  void rollback_to(void* mark);
  void reset();
  void release();
  std::size_t capacity() const noexcept;
  std::size_t used_bytes() const noexcept;
  std::size_t block_count() const noexcept;
  std::size_t block_size() const noexcept;
};
```

## Design notes
Round `size` up to `alignment`, align the bump pointer up; singly-linked chunk
list; `reset()` rewinds O(1) per chunk and re-links so the next allocation
reuses the oldest address. Single-threaded.

## Files
- Stub: `src/arena_allocator.cpp`
- Tests: `test/test_arena_allocator.cpp`
- Reference: `SOLUTION.md`