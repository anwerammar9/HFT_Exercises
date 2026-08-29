# Exercise 07 — Arena Allocator (Task)

## The problem (in plain words)

HFT per-message work (decoding a packet, building a snapshot, fan-out to
subscriptions) should **never touch `malloc/free` per object**. The scratch
layer is a **bump allocator**: big contiguous chunks are obtained from the OS,
objects are carved out of the current chunk by bumping a pointer forward, and
everything is reclaimed **in bulk** — `reset()` back to the start, `release()`
back to the OS, or `rollback_to(mark)` to a saved position. Individual frees do
not exist.

## Requirements (what the tests check)

1. `allocate(size, alignment)` returns at least `size` bytes aligned to
   `alignment`; live allocations **never overlap**; `allocate(0)` still returns
   a valid, unique, non-null pointer.
2. A request too big for the current chunk **chains a fresh chunk**.
3. `deallocate(p)` is a documented **no-op** — it must not corrupt state (all
   memory comes back via bulk reclaim).
4. `reset()` rewinds to the start of the **first** chunk. Previously handed-out
   memory is reusable and the **next allocation lands on the very same
   address** (no OS round-trip); `used_bytes() == 0`, `capacity()` unchanged,
   chunks retained.
5. `release()` frees every chunk back to the OS. The arena is brand new
   (`capacity() == 0`, `block_count() == 0`) yet still usable.
6. `allocate_all()` “commits” everything so far and returns a **mark**;
   `rollback_to(mark)` undoes **only** the allocations made after the mark —
   strict stack discipline. Marks can be nested and taken repeatedly.
7. `capacity()` / `used_bytes()` / `block_count()` report bytes owned, bytes
   handed out (live, summed across chunks), and chunk count.

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

## How to think about it (suggested design)

- One `struct Block` holding a raw byte array + a bump offset + a `next` link,
  forming a singly-linked list; `head_` is the chunk allocations come from.
- `allocate`: **align up** — round `size` up to the alignment (e.g.
  `(size + alignment − 1) & ~(alignment − 1)`) and bump the pointer forward to
  the next aligned address; if the current block can't hold the result, chain
  a new block.
- `reset()` must rewind in O(1) per chunk and **re-link** so the next
  allocation reuses the oldest address (walk the list once, zero every offset).
- `allocations` after a mark: a mark is just the current `(block, offset)`;
  `rollback_to(mark)` rewinds the bump pointer of every block created *after*
  the mark's block and frees them.
- Single-threaded by contract. This is pointer + loop + linked-list work — no
  templates.

## Make it harder (optional — not covered by the tests)

- **Scratch API polish:** add `malloc_usable_bytes`-style introspection
  (largest contiguously available today, waste %) and an `owns(void*)` check.
- **Alignment for over-aligned types:** verify `allocate(n, 64)` against
  `alignas(64)` real structs in a stress loop.
- **Thread-local arenas:** a `thread_local` arena per thread + per-message
  marks, so a whole library uses scratch without any shared state.
- **Hybrid with 08:** build an object-pool-on-top-of-arena and show the two
  bulk-reclaim strategies cooperating.

## Files

- Stub: `src/arena_allocator.cpp`
- Tests: `test/test_arena_allocator.cpp`
- Reference: `SOLUTION.md`