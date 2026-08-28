#include "object_pool.h"

#include <string>

// Explicit instantiations: guarantees the header (stubs included) really
// compiles for both a POD type and a std type.
template class ObjectPool<int>;
template class ObjectPool<std::string>;