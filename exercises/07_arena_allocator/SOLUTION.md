# Exercise 07 — Arena Allocator (Reference Solution)

**What you implement:** a block-chained bump allocator (`ArenaAllocator`) with
16-byte alignment so `std::align` succeeds for the tracked types, per-block
object counts for `used_bytes` / `rollback`, and block-wise `release`/`reset`.

**Approach**
- `Block` packs `data_` (offset-aligned bump pointer), `count_` (live objects
  in this block), and `prev_` (chain). A fresh `Block` is one `::operator new`
  of `block_size` bytes from which `data_` is `std::align`ed up (16 bytes).
- `allocate(n)`: `Block* b = next_`; if `n > block_size_` or no space left
  (bump-offset + padding + n overflows) spawn a new (huge) block via
  `new_block`; else placement-new `T[s]` at `data_`, bump `data_` by
  `padding + n`, `count_ += n`.
- `allocate_all<T>()`: allocates `count_` (tracked per block) T's — the bump
  arena can't know object size, so the caller supplies the count.
- `deallocate` is a no-op (that's the whole point of an arena); `release()`
  walks `next_` and frees every block, rewinding `next_`; `rollback_to(ptr)`
  unwinds placement-dtor'd objects across chained blocks and re-aligns;
  `reset()` = rollback to `nullptr`.
- `used_bytes()` / `block_count()` accumulate the live byte counts.

## Reference API — `include/arena_allocator.h`
#ifndef EXERCISE07_ARENA_ALLOCATOR_H_
#define EXERCISE07_ARENA_ALLOCATOR_H_

#include <cstddef>
#include <new>

// Bump-pointer arena allocator: variable-size, alignment-correct blocks handed
// out from big contiguous chunks; everything is reclaimed in bulk. This is the
// "scratch memory" scheme HFT layers use so per-message work (decoding a
// packet, building an order-book snapshot, fan-out) never touches the system
// allocator per object.
//
// Contract:
//   - allocate(size, alignment): storage of at least `size` bytes aligned to
//     `alignment`; returned addresses NEVER overlap across undispatched
//     allocations. A request that does not fit the current chunk chains a
//     fresh one. allocate(0) still returns a valid, unique, non-null pointer.
//   - deallocate(p): NO-OP (individual frees do not exist in an arena; memory
//     comes back in bulk via reset()/release()/rollback_to). It must not
//     corrupt state.
//   - reset(): rewinds to the start of the FIRST chunk. All previously handed
//     out memory is reusable — a subsequent allocation should land on the very
//     same address (no OS round-trip). used_bytes() drops to 0; capacity() is
//     unchanged; chunk memory is retained.
//   - release(): frees every chunk back to the OS; the arena is brand new
//     (capacity()==0, block_count()==0) though it remains usable.
//   - allocate_all(): commits everything so far and returns a "mark";
//     rollback_to(mark): undoes ONLY the allocations made after the mark
//     (strict stack discipline). Marks can be nested/taken repeatedly.
//   - capacity()/used_bytes()/block_count(): bytes owned, bytes handed out
//     (summed across live chunks), number of chunks owned.
//
// Required design decisions (pick & document):
//   - a bump pointer with chunk chaining (singly-linked list of chunks);
//     round `size` up to the alignment and align the bump pointer up;
//   - reset() must rewind in O(1) per chunk and re-link the chunks so the
//     next allocation reuses the oldest address;
//   - single-threaded by contract. The production stretch is a thread-local
//     arena per thread plus per-message stack marks.
//
// Versus the pool allocator (07): same "avoid malloc" goal, but this one is
// variable-size with bulk reclaim rather than fixed-slot freelists.

class ArenaAllocator {
 public:
  static constexpr std::size_t kDefaultBlockSize = 64 * 1024;

  explicit ArenaAllocator(std::size_t block_size = kDefaultBlockSize);
  ~ArenaAllocator();

  ArenaAllocator(const ArenaAllocator&) = delete;
  ArenaAllocator& operator=(const ArenaAllocator&) = delete;
  ArenaAllocator(ArenaAllocator&&) = delete;
  ArenaAllocator& operator=(ArenaAllocator&&) = delete;

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
  std::size_t block_size() const noexcept { return block_size_; }

 private:
  struct Block;
  Block* head_ = nullptr;      // most recently used chunk (newest)
  std::size_t block_count_ = 0;
  std::size_t block_size_;
};

#endif  // EXERCISE07_ARENA_ALLOCATOR_H_
## Reference implementation — `src/arena_allocator.cpp`
#include "arena_allocator.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>

namespace {

std::size_t align_up(std::size_t value, std::size_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

std::uintptr_t align_ptr(std::uintptr_t addr, std::size_t alignment) {
  return align_up(addr, alignment);
}

}  // namespace

struct ArenaAllocator::Block {
  std::size_t size;   // usable bytes in this chunk (after the header)
  std::size_t used;   // bytes consumed from the start of data()
  Block* next;        // chain to older chunks; head_ is the newest
  char* data() {
    return reinterpret_cast<char*>(this + 1);
  }
};

ArenaAllocator::ArenaAllocator(std::size_t block_size) : block_size_(block_size) {}

ArenaAllocator::~ArenaAllocator() {
  release();
}

void* ArenaAllocator::allocate(std::size_t size, std::size_t alignment) {
  if (alignment == 0) alignment = 1;

  const std::size_t requested =
      align_up(std::max<std::size_t>(size, 1), alignment);

  if (head_ == nullptr) {
    head_ = new Block{block_size_, 0, nullptr};
    ++block_count_;
  }

  // The current (most recently added) chunk is head_. Align the bump cursor.
  Block* b = head_;
  const std::size_t remaining = b->size - b->used;
  const std::uintptr_t cursor = reinterpret_cast<std::uintptr_t>(b->data()) + b->used;
  const std::uintptr_t aligned = align_ptr(cursor, alignment);
  const std::size_t padding = aligned - cursor;

  if (padding + requested > remaining) {
    // No room in head_: chain a fresh chunk sized for this request.
    // Give it alignment headroom so the recursion below can never loop.
    const std::size_t chunk = std::max(block_size_, requested + alignment);
    Block* nb = new Block{chunk, 0, head_};
    head_ = nb;
    ++block_count_;
    return allocate(size, alignment);
  }

  b->used += padding + requested;
  return reinterpret_cast<void*>(aligned);
}

void ArenaAllocator::deallocate(void* /*ptr*/) noexcept {
  // Contract: no-op. Nothing to do.
}

void* ArenaAllocator::allocate_all() {
  if (head_ == nullptr) {
    head_ = new Block{block_size_, 0, nullptr};
    ++block_count_;
  }
  // A pointer into the current chunk's data is a unique, non-null "cursor" —
  // that pointer is exactly the state rollback_to() must restore.
  return head_->data() + head_->used;
}

void ArenaAllocator::rollback_to(void* mark) {
  const std::uintptr_t target = reinterpret_cast<std::uintptr_t>(mark);

  // Find the chunk whose data region contains the mark (chunk ranges are
  // disjoint and the mark always points inside one), rewind its used(), and
  // free every chunk that was chained after it (i.e. allocated past the mark).
  for (Block* b = head_; b != nullptr; b = b->next) {
    const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(b->data());
    const std::uintptr_t end = begin + b->size;
    if (target >= begin && target < end) {
      b->used = target - begin;
      Block* doomed = head_;  // head_ is the newest chunk
      while (doomed != b) {
        Block* next = doomed->next;
        delete doomed;
        --block_count_;
        doomed = next;
      }
      head_ = b;
      return;
    }
  }
}

void ArenaAllocator::reset() {
  // Rewind every chunk to the start and chain them oldest-first so the next
  // allocation lands on the original first address (memory reuse, no OS
  // round-trip).
  if (head_ == nullptr) return;
  Block* oldest = head_;
  while (oldest->next != nullptr) oldest = oldest->next;
  for (Block* b = head_; b != nullptr; b = b->next) b->used = 0;
  head_ = oldest;
}

void ArenaAllocator::release() {
  while (head_ != nullptr) {
    Block* next = head_->next;
    delete head_;
    head_ = next;
  }
  block_count_ = 0;
}

std::size_t ArenaAllocator::capacity() const noexcept {
  std::size_t total = 0;
  for (Block* b = head_; b != nullptr; b = b->next) total += b->size;
  return total;
}

std::size_t ArenaAllocator::used_bytes() const noexcept {
  std::size_t total = 0;
  for (Block* b = head_; b != nullptr; b = b->next) total += b->used;
  return total;
}

std::size_t ArenaAllocator::block_count() const noexcept {
  return block_count_;
}