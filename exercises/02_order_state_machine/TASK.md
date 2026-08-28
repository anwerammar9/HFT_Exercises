# Exercise 02 — Order State Machine (Task)

## Problem
Implement a strict order-lifecycle state machine whose transitions are safe
under **concurrent** fill/cancel messages: two racing callers must resolve to
exactly one winner, and no reader may ever observe an illegal state.

## Requirements (what the tests check)
1. `transition(to)` returns `true` only if the edge `current → to` is legal,
   and then commits. For any illegal edge it returns `false` and leaves the
   state **unchanged**.
2. Legal edges:
   - `New` → {`PendingNew`, `PartiallyFilled`, `Cancelled`, `Rejected`}
   - `PendingNew` → {`New`, `PartiallyFilled`, `Rejected`}
   - `PartiallyFilled` → {`PartiallyFilled`, `Filled`, `PendingCancel`, `Cancelled`}
   - `PendingCancel` → {`Cancelled`, `PartiallyFilled`, `Filled`}
   - `Filled` / `Cancelled` / `Rejected` are **terminal** (no outgoing edges).
3. `state()` always returns a state the machine actually reached — never torn.
4. Race test: two threads target the same order simultaneously; exactly one
   transition wins. (Solution: back `state_` with ONE `std::atomic<OrderState>`
   and commit every transition with a single CAS loop — load, check `legal()`,
   `compare_exchange`.)

## Public API
```cpp
enum class OrderState { New, PendingNew, PartiallyFilled, Filled,
                        PendingCancel, Cancelled, Rejected };

class OrderStateMachine {
  bool transition(OrderState to);
  OrderState state() const noexcept;
};
```

## Files
- Stub: `src/order_state_machine.cpp`
- Tests: `test/test_order_state_machine.cpp`
- Reference: `SOLUTION.md`