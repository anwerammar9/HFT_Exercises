#include "pool_allocator.h"

#include <cstdint>
#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers this
// title). Keeping this TU pins the instantiations exercised by the unit tests.

template class PoolAllocator<std::uint64_t>;
template class PoolAllocator<std::string>;