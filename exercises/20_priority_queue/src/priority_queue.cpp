#include "priority_queue.h"

#include <string>

// Explicit instantiations: header (stubs included) must really compile for
// both a trivially-copyable type (int) with the default Compare and for a
// heap-string type with a custom comparator.
template class PriorityQueue<int>;
template class PriorityQueue<std::string, std::greater<std::string>>;