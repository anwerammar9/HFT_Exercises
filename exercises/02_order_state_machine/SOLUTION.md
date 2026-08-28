# Exercise 02 — Order State Machine (Reference Solution)

**What you implement:** a strict order-lifecycle state machine safe for
concurrent fill/cancel messages from an OMS — every transition gated by one
atomic compare-exchange so two racing transitions can't land in an
inconsistent state.

**Approach**
- State lives in a single `std::atomic<OrderState>` (initial `New`).
- `transition(to)`: load the current state, consult the `legal()` table, then
  `compare_exchange_weak`. Exactly one of two concurrent transitions wins; the
  loser's `from` is auto-refreshed by the failed CAS and the loop simply
  re-checks legality against the new reality.
- Legal edges: `New -> {PendingNew, PartiallyFilled, Cancelled, Rejected}`,
  `PendingNew -> {New, PartiallyFilled, Rejected}`, `PartiallyFilled ->
  {PartiallyFilled, Filled, PendingCancel, Cancelled}`, `PendingCancel ->
  {Cancelled, PartiallyFilled, Filled}`; `Filled`/`Cancelled`/`Rejected` are
  terminal (no outgoing edges).
- `state()` is a plain relaxed-acquire load: readers observe a state this
  machine actually reached — never a torn/impossible value, since commit is an
  atomic single word.

## Reference API — `include/order_state_machine.h`
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

## Reference implementation — `src/order_state_machine.cpp`
#include "order_state_machine.h"

// Every edge is resolved with ONE compare-exchange: load the current state,
// consult the legal() table, then CAS-commit. Exactly one of two racing
// transitions wins; the loser's CAS fails and it returns false (the state it
// read is then stale — the loop simply re-reads and re-checks).

bool OrderStateMachine::legal(OrderState from, OrderState to) noexcept {
  switch (from) {
    case OrderState::New:
      return to == OrderState::PendingNew ||
             to == OrderState::PartiallyFilled ||
             to == OrderState::Cancelled ||
             to == OrderState::Rejected;
    case OrderState::PendingNew:
      return to == OrderState::New ||
             to == OrderState::PartiallyFilled ||
             to == OrderState::Rejected;
    case OrderState::PartiallyFilled:
      return to == OrderState::PartiallyFilled ||
             to == OrderState::Filled ||
             to == OrderState::PendingCancel ||
             to == OrderState::Cancelled;
    case OrderState::PendingCancel:
      return to == OrderState::Cancelled ||
             to == OrderState::PartiallyFilled ||
             to == OrderState::Filled;
    case OrderState::Filled:
    case OrderState::Cancelled:
    case OrderState::Rejected:
      return false;  // terminal: no outgoing edges
  }
  return false;
}

bool OrderStateMachine::transition(OrderState to) {
  OrderState from = state_.load(std::memory_order_relaxed);
  while (true) {
    if (!legal(from, to)) return false;
    if (state_.compare_exchange_weak(from, to, std::memory_order_acq_rel,
                                     std::memory_order_acq_rel)) {
      return true;
    }
    // CAS failed: `from` now holds the actual current state; retry against it.
  }
}