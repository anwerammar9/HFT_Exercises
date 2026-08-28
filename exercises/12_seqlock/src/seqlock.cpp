#include "seqlock.h"

#include <cstdint>

// Template implementations live inline in the header (that's where the user's
// TODO(anwer) work happens for Seqlock). This TU exists so the library target
// compiles and the explicit instantiations below are exercised — swap these out
// if you rearchitect the template.

template class Seqlock<std::uint64_t>;
template class Seqlock<std::uint32_t>;

// TODO(anwer): RWSpinLock implementation lives in src/rw_spinlock.cpp.