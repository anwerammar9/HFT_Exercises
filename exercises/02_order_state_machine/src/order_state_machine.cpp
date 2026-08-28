#include "order_state_machine.h"

// TODO(anwer): implement the real machine (see SOLUTION.md).
//
// Suggested shape: CAS loop — load `state_`, consult `legal()` below, then
// compare_exchange the new state; the transition COMMITS only if the CAS
// succeeds (a concurrent winner invalidates our attempt).

bool OrderStateMachine::legal(OrderState /*from*/, OrderState /*to*/) noexcept {
  return false;
}

bool OrderStateMachine::transition(OrderState /*to*/) {
  return false;  // stub: an always-New machine that moves nowhere
}