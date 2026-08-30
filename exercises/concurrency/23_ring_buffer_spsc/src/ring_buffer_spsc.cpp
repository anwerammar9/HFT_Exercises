#include "ring_buffer_spsc.h"

#include <cstdint>

// The SpscRingBuffer implementation is inline in the header (see the
// TODO(anwer) markers there). This TU keeps the library target and pins the
// template instantiations used by the tests so a stub linker error can't hide
// an implementation gap.

template class SpscRingBuffer<std::uint64_t, 1024>;
template class SpscRingBuffer<std::uint32_t, 8>;