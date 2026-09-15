#include "unique_ptr.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers once
// this file is rolled back to practice mode). Keeping this TU pins the
// instantiations exercised by the unit tests.

template class UniquePtr<int>;
template class UniquePtr<std::string>;
template class UniquePtr<int[]>;
