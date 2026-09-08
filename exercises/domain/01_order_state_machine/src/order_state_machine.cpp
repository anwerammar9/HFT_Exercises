#include "order_state_machine.h"

// TODO(anwer): implement legal() + the CAS transition (see SOLUTION.md).
// Stub: no edge is legal, so the machine is stuck in New forever and every
// transition() returns false — the suites fail RED.

bool OrderStateMachine::legal(OrderState /*from*/, OrderState /*to*/) noexcept {
  return false;
}

bool OrderStateMachine::transition(OrderState /*to*/) {
  return false;  // stub: always reject
}