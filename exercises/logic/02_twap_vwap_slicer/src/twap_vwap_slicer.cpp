#include "twap_vwap_slicer.h"

#include <vector>

// TODO(anwer): implement plan() for both slicers (see SOLUTION.md).
// Stub: always produces the empty plan, so the tests fail RED (only the
// "invalid input -> empty plan" cases pass).

TwapSlicer::TwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
                       std::size_t n_slices)
    : total_qty_(total_qty), duration_(duration), n_slices_(n_slices) {}

VwapSlicer::VwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
                       std::vector<double> fractions)
    : total_qty_(total_qty), duration_(duration),
      fractions_(fractions) {}

std::vector<Slice> TwapSlicer::plan() const { return {}; }

std::vector<Slice> VwapSlicer::plan() const { return {}; }