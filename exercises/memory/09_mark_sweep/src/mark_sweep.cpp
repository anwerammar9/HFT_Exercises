#include "mark_sweep.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers once
// this file is rolled back to practice mode). Keeping this TU pins the
// instantiations exercised by the unit tests.

template class GcPtr<int>;
template class GcRooted<int>;
