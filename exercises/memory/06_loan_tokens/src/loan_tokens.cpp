#include "loan_tokens.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers once
// this file is rolled back to practice mode). Keeping this TU pins the
// instantiations exercised by the unit tests.

template class LoanBox<int>;
template class LoanBox<std::string>;
