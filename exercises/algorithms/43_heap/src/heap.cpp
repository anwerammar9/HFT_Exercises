#include "heap.h"

#include <string>
#include <vector>

// Explicit instantiations: guarantees the header (stubs included) really
// compiles for both POD- and heap-string types and both comparator directions.
template class BinaryHeap<int>;
template class BinaryHeap<int, std::greater<int>>;
template class BinaryHeap<std::string>;
template void heapsort(std::vector<int>&);
template void heapsort(std::vector<std::string>&);