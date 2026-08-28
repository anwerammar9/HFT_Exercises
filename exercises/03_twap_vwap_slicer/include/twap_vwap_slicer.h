#ifndef EXERCISE03_TWAP_VWAP_SLICER_H_
#define EXERCISE03_TWAP_VWAP_SLICER_H_

#include <chrono>
#include <cstdint>
#include <vector>

// TWAP / VWAP parent-order slicers.
//
// Contract (TwapSlicer):
//   - plan() splits `total_qty` into `n_slices` slices evenly spaced across
//     `duration`. send_at of slice i (0-based) = duration * (i+1) / n (ms).
//   - Rounding: base = total/n, remainder = total % n; the LAST slice absorbs
//     the remainder, so sum(slice qty) == total exactly.
//   - n == 1 -> one slice {total, 0ms} (send the whole parent immediately).
//   - n == 0 or total <= 0 -> empty plan.
//
// Contract (VwapSlicer):
//   - plan() weights slice size by per-bucket volume `fractions` (they need
//     not sum to 1; they are normalized). Integer rounding via largest
//     remainder, so the slices sum EXACTLY to total.
//   - send_at of slice i = duration * (normalized cumulative fraction through
//     slot i), i.e. the end of that volume bucket. Exactly one fraction ->
//     one slice {total, 0ms}.
//   - Empty / all-zero fractions or total <= 0 -> empty plan.
//
// TODO(anwer): implement plan() (see SOLUTION.md). Stub returns {} for both
// slicers, so the tests run RED.
using Qty = std::int64_t;

struct Slice {
  Qty qty;
  std::chrono::milliseconds send_at;
};

class TwapSlicer {
 public:
  TwapSlicer(Qty total_qty, std::chrono::milliseconds duration, std::size_t n_slices);

  std::vector<Slice> plan() const;

 private:
  Qty total_qty_{0};
  std::chrono::milliseconds duration_{0};
  std::size_t n_slices_{0};
};

class VwapSlicer {
 public:
  VwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
             std::vector<double> fractions);

  std::vector<Slice> plan() const;

 private:
  Qty total_qty_{0};
  std::chrono::milliseconds duration_{0};
  std::vector<double> fractions_;
};

#endif  // EXERCISE03_TWAP_VWAP_SLICER_H_