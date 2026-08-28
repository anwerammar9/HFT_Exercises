#include "lru_cache.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers).
// Keeping this TU pins the instantiations exercised by the unit tests.

template class LruCache<std::string, int>;
template class LruCache<int, std::string>;