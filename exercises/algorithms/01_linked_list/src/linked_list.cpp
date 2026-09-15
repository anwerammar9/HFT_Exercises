#include "linked_list.h"

#include <string>

// Explicit instantiations: guarantees the header (stubs included) really
// compiles for both a trivially-copyable type (int) and a heap-string type.
template class LinkedList<int>;
template class LinkedList<std::string>;