#include "arena_allocator.h"

#include <stdexcept>

// TODO(anwer): implement the bump-arena (see SOLUTION.md).
//
// Contract: allocate() hands out aligned bytes from a linked chain of big
// chunks (Block = {size, used, next}, data inline after the header); allocate
// of the current chunk's tail chains a fresh chunk; deallocate is a NO-OP;
// reset() rewinds chunk cursors so the next allocation reuses the original
// address; release() frees every chunk; rollback_to(mark) undoes allocations
// made after `mark` taken from allocate_all().

ArenaAllocator::ArenaAllocator(std::size_t block_size) : block_size_(block_size) {}

ArenaAllocator::~ArenaAllocator() { release(); }

void* ArenaAllocator::allocate(std::size_t /*size*/, std::size_t /*alignment*/) {
  throw std::logic_error("not implemented");  // TODO(anwer)
}

void ArenaAllocator::deallocate(void* /*ptr*/) noexcept {}

void* ArenaAllocator::allocate_all() {
  throw std::logic_error("not implemented");  // TODO(anwer)
}

void ArenaAllocator::rollback_to(void* /*mark*/) {
  throw std::logic_error("not implemented");  // TODO(anwer)
}

void ArenaAllocator::reset() {}

void ArenaAllocator::release() {}

std::size_t ArenaAllocator::capacity() const noexcept { return 0; }

std::size_t ArenaAllocator::used_bytes() const noexcept { return 0; }

std::size_t ArenaAllocator::block_count() const noexcept { return 0; }