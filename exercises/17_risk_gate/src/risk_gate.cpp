#include "risk_gate.h"

// TODO(anwer): implement the real gate (see SOLUTION.md).
//
// Suggested shape: CAS loop over the packed `state_` —
//   load cur; unpack notional (high 32) + position (low 32);
//   delta = (Order::Buy ? +qty : -qty); candidate notional = cur + price*qty;
//   if |position+delta| > max_position_ || candidate_notional > max_notional_
//       -> return false (reject, nothing reserved);
//   repack and compare_exchange; retry on failure (someone else committed).
// release(): unpack, XOR the same deltas back, CAS-commit.
// Accessors: unpack + sign-extend the low word.

RiskGate::RiskGate(Qty max_position, Notional max_notional)
    : max_position_(max_position), max_notional_(max_notional) {}

bool RiskGate::check_and_reserve(const Order& /*order*/) noexcept {
  return false;  // stub: reject everything
}

void RiskGate::release(const Order& /*order*/) noexcept {}

Qty RiskGate::position() const noexcept { return 0; }

Notional RiskGate::notional_used() const noexcept { return 0; }