#ifndef EXERCISE02_ORDER_STATE_MACHINE_H_
#define EXERCISE02_ORDER_STATE_MACHINE_H_

#include <atomic>

// Strict order state machine, safe for concurrent fill/cancel messages from
// an OMS.
//
// Contract:
//   - transition(to) applies `from -> to` iff that edge is in the legal table;
//     returns false and leaves the state UNCHANGED otherwise.
//   - Legal edges:
//       New             -> { PendingNew, PartiallyFilled, Cancelled, Rejected }
//       PendingNew      -> { New, PartiallyFilled, Rejected }
//       PartiallyFilled -> { PartiallyFilled, Filled, PendingCancel, Cancelled }
//       PendingCancel   -> { Cancelled, PartiallyFilled, Filled }
//       Filled / Cancelled / Rejected are TERMINAL (no outgoing edges).
//   - state() always returns a state this machine actually reached; a reader
//     can never observe a torn / illegal state.
//
// Implementation notes (reference solution):
//   - Back the state with ONE `std::atomic<OrderState>` and resolve every
//     transition with a single CAS: load, consult the legal() table, then
//     compare_exchange. Exactly one of two racing transitions wins; the loser's
//     CAS fails and it returns false. No lock required.
//
// TODO(anwer): implement the real machine (see SOLUTION.md). The stub rejects
// every transition (an always-New machine), so the suites run RED.
enum class OrderState {
  New,
  PendingNew,
  PartiallyFilled,
  Filled,
  PendingCancel,
  Cancelled,
  Rejected,
};

class OrderStateMachine {
 public:
  OrderStateMachine() = default;

  // Returns true iff `from == state()` and the edge is legal; then commits.
  bool transition(OrderState to);

  OrderState state() const noexcept {
    return state_.load(std::memory_order_acquire);
  }

 private:
  static bool legal(OrderState from, OrderState to) noexcept;

  std::atomic<OrderState> state_{OrderState::New};
};

#endif  // EXERCISE02_ORDER_STATE_MACHINE_H_