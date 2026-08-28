#include "twap_vwap_slicer.h"

#include <stdexcept>

// TODO(anwer): implement plan() (see SOLUTION.md).
//
// TWAP: base = total/n, remainder to the LAST slice (sum == total exactly);
//       send_at(i) = duration*(i+1)/n ms; n == 1 -> {total, 0ms}; n == 0 or
//       total <= 0 -> {}.
// VWAP: normalize fractions, largest-remainder integer rounding (sum == total
//       exactly); send_at(i) = duration * cum_norm_fraction (end of bucket);
//       one fraction -> {total, 0ms}; empty/all-zero or total <= 0 -> {}.

TwapSlicer::TwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
                       std::size_t n_slices)
    : total_qty_(total_qty), duration_(duration), n_slices_(n_slices) {}

VwapSlicer::VwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
                       std::vector<double> fractions)
    : total_qty_(total_qty), duration_(duration), fractions_(std::move(fractions)) {}

std::vector<Slice> TwapSlicer::plan() const {
  throw std::logic_error("not implemented");  // stub
}

std::vector<Slice> VwapSlicer::plan() const {
  throw std::logic_error("not implemented");  // stub
}