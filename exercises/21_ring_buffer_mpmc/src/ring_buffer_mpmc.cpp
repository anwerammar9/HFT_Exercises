#include "ring_buffer_mpmc.h"

#include <cstdint>

// Implementation is inline in the header (see the TODO(anwer) markers).
// Keeping this TU pins the instantiations exercised by the unit tests.

template class MpmcQueue<int>;
template class MpmcQueue<std::uint64_t>;