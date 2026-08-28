#include "dynamic_vector.h"

#include <memory>
#include <string>

// Explicit instantiations: guaranteed-compiles for plain, noexcept-movable,
// and non-trivially-copyable element types.
template class Vector<int>;
template class Vector<double>;
template class Vector<std::string>;
template class Vector<std::unique_ptr<int>>;