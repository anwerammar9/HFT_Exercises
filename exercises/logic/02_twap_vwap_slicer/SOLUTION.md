# Exercise logic/02_twap_vwap_slicer (ex03) — TWAP / VWAP Slicer (Reference Solution)

**What you implement:** slice a parent order into child orders — time-weighted
(TWAP, evenly spaced) or volume-weighted (VWAP, proportional to forecast
per-bucket volume) — with integer rounding that guarantees the slices sum
**exactly** to the parent size.

**Approach**
- **TWAP:** `base = total / n`, `rem = total % n`; the last child absorbs `rem`
  (so `Σ qty == total` exactly). `send_at(i) = duration · (i+1) / n` (integer
  ms). `n == 1` → one slice `{total, 0ms}` (execute the parent immediately).
- **VWAP:** normalize the volume fractions to sum to 1, then distribute `total`
  by **largest remainder**: start with `qty[i] = trunc(total · f_i / Σf)`,
  and hand the leftover units (0..n−1) out one at a time to the slots with the
  highest fractional part. Sum is exact by construction.
- **VWAP timing:** slice `i` is sent at the end of its volume bucket —
  `duration · (cumulative normalized fraction through slot i)`. One fraction →
  one slice `{total, 0ms}` (contract); else the final slice lands exactly at
  `duration` (cumulative = 1.0).
- Degenerate inputs (`n == 0`, `total <= 0`, empty/all-zero fractions) → empty
  plan.

## Reference API — `include/twap_vwap_slicer.h`
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

## Reference implementation — `src/twap_vwap_slicer.cpp`
#include "twap_vwap_slicer.h"

#include <numeric>

TwapSlicer::TwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
                       std::size_t n_slices)
    : total_qty_(total_qty), duration_(duration), n_slices_(n_slices) {}

VwapSlicer::VwapSlicer(Qty total_qty, std::chrono::milliseconds duration,
                       std::vector<double> fractions)
    : total_qty_(total_qty), duration_(duration), fractions_(std::move(fractions)) {}

// TWAP: evenly spaced child orders. Base = total/n, remainder spread
// LARGEST-REMAINDER style but here the last slice simply absorbs it, so
// sum(slice) == total exactly. send_at of slice i = duration*(i+1)/n (ms).

std::vector<Slice> TwapSlicer::plan() const {
  std::vector<Slice> out;
  if (n_slices_ == 0 || total_qty_ <= 0) return out;

  const Qty base = total_qty_ / static_cast<Qty>(n_slices_);
  const Qty rem = total_qty_ % static_cast<Qty>(n_slices_);
  out.reserve(n_slices_);

  if (n_slices_ == 1) {
    // Contract: n == 1 -> one slice {total, 0ms} (send immediately).
    return std::vector<Slice>{Slice{base + rem, std::chrono::milliseconds{0}}};
  }

  for (std::size_t i = 0; i < n_slices_; ++i) {
    Qty qty = base + (i + 1 == n_slices_ ? rem : 0);
    std::chrono::milliseconds dur{};
    // send_at = duration * (i+1) / n (integer ms, per contract)
    const auto t = static_cast<std::int64_t>(i + 1);
    const auto n = static_cast<std::int64_t>(n_slices_);
    dur = std::chrono::milliseconds{duration_.count() * t / n};
    out.push_back(Slice{qty, dur});
  }
  return out;
}

// VWAP: slice proportional to per-bucket volume. Normalize fractions so they
// sum to 1; the largest-remainder method distributes total EXACTLY.

std::vector<Slice> VwapSlicer::plan() const {
  std::vector<Slice> out;
  if (fractions_.empty() || total_qty_ <= 0) return out;

  const double sum =
      std::accumulate(fractions_.begin(), fractions_.end(), 0.0);
  if (sum <= 0.0) return out;  // all-zero fractions

  const std::size_t n = fractions_.size();
  std::vector<double> ideal(n);
  std::vector<Qty> qty(n, 0);

  Qty floor_sum = 0;
  for (std::size_t i = 0; i < n; ++i) {
    ideal[i] = total_qty_ * (fractions_[i] / sum);
    qty[i] = static_cast<Qty>(ideal[i]);  // truncate toward zero
    floor_sum += qty[i];
  }

  Qty leftover = total_qty_ - floor_sum;  // 0..n-1 by construction
  while (leftover > 0) {
    // largest remainder: the slot with the highest fractional part gets one.
    std::size_t best = 0;
    double best_frac = -1.0;
    for (std::size_t i = 0; i < n; ++i) {
      const double frac = ideal[i] - static_cast<double>(qty[i]);
      if (frac > best_frac + 1e-12) {
        best_frac = frac;
        best = i;
      }
    }
    ++qty[best];
    --leftover;
  }

  if (n == 1) {
    // Contract: exactly one fraction -> one slice {total, 0ms}.
    return std::vector<Slice>{Slice{total_qty_, std::chrono::milliseconds{0}}};
  }

  // Send at the END of each volume bucket: cumulative normalized fraction.
  double cum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    cum += fractions_[i] / sum;
    const auto dur = std::chrono::milliseconds{
        static_cast<std::int64_t>(duration_.count() * cum)};
    out.push_back(Slice{qty[i], dur});
  }
  return out;
}