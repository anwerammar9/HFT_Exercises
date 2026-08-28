#include "hash_map.h"

#include <string>

// Explicit instantiations: guarantees the header (stubs included) really
// compiles for both POD- and std-types.
template class HashMap<int, int>;
template class HashMap<std::string, std::string>;
template class HashMap<int, std::string>;