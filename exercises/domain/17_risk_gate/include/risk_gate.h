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