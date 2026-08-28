#include "lockfree_stack.h"

#include <cstdint>
#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers).
// Keeping this TU pins the instantiations exercised by the unit tests.

template class LockFreeStack<int>;
template class LockFreeStack<std::uint64_t>;
template class LockFreeStack<std::string>;