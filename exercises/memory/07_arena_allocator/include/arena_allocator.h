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