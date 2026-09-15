# Exercise domain/03_risk_gate (ex17) — Pre-Trade Risk Gate (Reference Solution)

**What you implement:** an atomic position/notional limit checker — the
classic *check-then-act* race, where a naive
`if (would_exceed_limit()) return false; position += qty;` breaks under
concurrent callers. Solved with an optimistic CAS loop, not a coarse lock.

**Approach**
- Both counters ride in ONE 64-bit atomic `state_`: `notional_used` in the
  upper 32 bits, signed `position` in the lower 32 (a packed design that keeps
  the commit atomic; position/notional assumed to fit in int32).
- `check_and_reserve`: CAS loop — load, unpack, compute candidate
  (`new_position = position ± qty`, `new_notional = notional + price·qty`),
  reject on `|new_position| > max_position_` or `new_notional >
  max_notional_` (or a negative notional), else CAS-commit. A concurrent
  commit invalidates our expectation → the failed CAS refreshes `cur` and we
  retry. Rejects reserve nothing.
- `release`: CAS loop subtracting the same deltas (notional clamped at ≥ 0);
  frees capacity for later orders.
- Notional is **gross** (buys + sells both accrue), position is signed (a sell
  reduces a long) — that's what the boundary/accrual tests assert.

## Reference API — `include/risk_gate.h`
#ifndef EXERCISE17_RISK_GATE_H_
#define EXERCISE17_RISK_GATE_H_

#include <atomic>
#include <cstdint>

// Pre-trade risk gate: atomically reserves position + notional capacity.
//
// Contract:
//   - check_and_reserve(order) is an atomic check-then-commit. It succeeds iff
//     BOTH limits still hold AFTER the order were applied:
//         |position_curr + delta| <= max_position   (delta = +qty buy, -qty sell)
//         notional_curr + price*qty <= max_notional (notional is GROSS: buys + sells)
//     On success the counters are committed and true returned; otherwise
//     nothing changes and false is returned.
//   - release(order) returns the reserved capacity (order was rejected /
//     cancelled downstream); safe to call more than once (idempotent no-op by
//     net effect only if the caller tracks exactly once).
//   - position() / notional_used() report the committed values.
//
// Implementation notes (reference solution):
//   - The classic check-then-act race: a naive read-check-write is NOT atomic
//     under concurrent callers. Pack BOTH counters into one 64-bit atomic
//     (notional_used in the upper 32 bits, signed position in the lower 32)
//     and do a single compare-exchange loop: load, compute the candidate,
//     check both limits, CAS-commit. Optimistic retry, no coarse lock.
//   - Position/notional must fit in int32 for the packed layout.
//
// TODO(anwer): implement the gate (see SOLUTION.md). The stub rejects every
// order (position()/notional_used() stay 0), so the suites run RED.
using Qty = std::int64_t;
using Price = std::int64_t;
using Notional = std::int64_t;

enum class Side { Buy, Sell };

struct Order {
  std::int64_t id;
  Side side;
  Price price;
  Qty qty;
};

class RiskGate {
 public:
  RiskGate(Qty max_position, Notional max_notional);

  bool check_and_reserve(const Order& order) noexcept;
  void release(const Order& order) noexcept;

  Qty position() const noexcept;
  Notional notional_used() const noexcept;

 private:
  std::atomic<std::uint64_t> state_{0};  // high: notional_used, low: position
  Qty max_position_{0};
  Notional max_notional_{0};
};

#endif  // EXERCISE17_RISK_GATE_H_

## Reference implementation — `src/risk_gate.cpp`
#include "risk_gate.h"

#include <algorithm>
#include <cstdint>

// Both counters ride in ONE 64-bit atomic: notional_used in the upper 32
// bits, signed position in the lower 32. A check-then-commit is a single
// CAS loop — load, compute the candidate, verify both limits, commit. A
// concurrent reserve/release merely fails our CAS and we retry against the
// fresh value. No coarse lock.

namespace {

std::int64_t sign_extend(std::uint32_t lo) {
  return static_cast<std::int64_t>(static_cast<std::int32_t>(lo));
}

std::uint64_t pack(std::int64_t position, std::int64_t notional) {
  return (static_cast<std::uint64_t>(position) & 0xFFFFFFFFu) |
         (static_cast<std::uint64_t>(notional) << 32);
}

}  // namespace

RiskGate::RiskGate(Qty max_position, Notional max_notional)
    : max_position_(max_position), max_notional_(max_notional) {}

bool RiskGate::check_and_reserve(const Order& order) noexcept {
  const std::int64_t pos_delta = order.side == Side::Buy ? order.qty : -order.qty;
  const std::int64_t notional_delta = order.price * order.qty;

  std::uint64_t cur = state_.load(std::memory_order_relaxed);
  for (;;) {
    const std::int64_t position = sign_extend(static_cast<std::uint32_t>(cur));
    const std::int64_t notional = static_cast<std::int64_t>(cur >> 32);

    const std::int64_t new_position = position + pos_delta;
    const std::int64_t new_notional = notional + notional_delta;

    if (new_position > max_position_ || new_position < -max_position_ ||
        new_notional > max_notional_ || new_notional < 0) {
      return false;  // would breach a limit: reject, reserve nothing
    }

    if (state_.compare_exchange_weak(cur, pack(new_position, new_notional),
                                     std::memory_order_acq_rel,
                                     std::memory_order_relaxed)) {
      return true;
    }
    // failed CAS: `cur` was refreshed to the committed value; retry.
  }
}

void RiskGate::release(const Order& order) noexcept {
  const std::int64_t pos_delta = order.side == Side::Buy ? -order.qty : order.qty;
  const std::int64_t notional_delta = -(order.price * order.qty);

  std::uint64_t cur = state_.load(std::memory_order_relaxed);
  for (;;) {
    const std::int64_t position = sign_extend(static_cast<std::uint32_t>(cur));
    const std::int64_t notional = static_cast<std::int64_t>(cur >> 32);

    const std::int64_t new_position = position + pos_delta;
    const std::int64_t new_notional =
        std::max<std::int64_t>(0, notional + notional_delta);

    if (state_.compare_exchange_weak(cur, pack(new_position, new_notional),
                                     std::memory_order_acq_rel,
                                     std::memory_order_relaxed)) {
      return;
    }
  }
}

Qty RiskGate::position() const noexcept {
  return sign_extend(
      static_cast<std::uint32_t>(state_.load(std::memory_order_acquire)));
}

Notional RiskGate::notional_used() const noexcept {
  return static_cast<std::int64_t>(
      state_.load(std::memory_order_acquire) >> 32);
}