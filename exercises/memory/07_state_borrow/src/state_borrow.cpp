#include "state_borrow.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers once
// this file is rolled back to practice mode). Keeping this TU pins the
// instantiations exercised by the unit tests.

template class SBox<int>;
template class SBox<std::string>;
template class SRef<int>;
template class SMut<int>;
template class LockedBox<int>;
