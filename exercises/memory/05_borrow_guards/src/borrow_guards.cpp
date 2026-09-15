#include "borrow_guards.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers once
// this file is rolled back to practice mode). Keeping this TU pins the
// instantiations exercised by the unit tests.

template class BorrowBox<int>;
template class BorrowBox<std::string>;
template class BorrowRef<int>;
template class BorrowMut<int>;
