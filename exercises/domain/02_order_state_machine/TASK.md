# Exercise 02 — Order State Machine (Task)

## The problem (in plain words)

An order has a lifecycle: it is created, it may go to an exchange, it may fill
partially, and eventually it ends in `Filled`, `Cancelled` or `Rejected`. In
production the messages that drive an order arrive **from several threads at
once** — a fill report and a cancel report can race. The state machine must
accept exactly the *legal* transitions, silently reject everything else, and —
critically — when two transitions race, **exactly one** wins and no reader ever
observes a torn or illegal state.

## Requirements (what the tests check)

1. `transition(to)` returns `true` only if the edge `current → to` is legal; it
   then commits and returns `true`. For **any illegal edge** (including any
   edge out of a terminal state) it returns `false` and leaves the state
   **unchanged**.
2. Legal edges:
   - `New` → {`PendingNew`, `PartiallyFilled`, `Cancelled`, `Rejected`}
   - `PendingNew` → {`New`, `PartiallyFilled`, `Rejected`}
   - `PartiallyFilled` → {`PartiallyFilled`, `Filled`, `PendingCancel`, `Cancelled`}
   - `PendingCancel` → {`Cancelled`, `PartiallyFilled`, `Filled`}
   - `Filled` / `Cancelled` / `Rejected` are **terminal** — no outgoing edges.
3. `state()` always returns a state the machine actually reached — never a
   half-applied transition.
4. Race test: two threads call `transition` on the same order at the same time;
   **exactly one** succeeds. The loser must observe that the state already
   moved and return `false`.

## Public API

```cpp
enum class OrderState { New, PendingNew, PartiallyFilled, Filled,
                        PendingCancel, Cancelled, Rejected };

class OrderStateMachine {
  bool transition(OrderState to);
  OrderState state() const noexcept;
};
```

## How to think about it (suggested design)

- Back the state with **one** `std::atomic<OrderState>`. Commit every
  transition with a single CAS loop, with no lock:
  1. `cur = state_.load()`;
  2. if `legal(cur, to)` is false → return `false` (nothing to do);
  3. `compare_exchange(cur, to)` — if it succeeds, we won, return `true`;
  4. otherwise somebody else changed the state → loop back and re-check the new
     `cur` against the table.
- Write `legal(from, to)` as a plain table (`switch`, or a small array of legal
  targets indexed by `from`) — no template gymnastics.
- The `enum class` of 7 values fits in a few bits; the atomic stores it whole,
  so a reader always sees one complete state.

## Make it harder (optional — not covered by the tests)

- **Audit trail:** keep a compact circular history of `(from, to, logical
  timestamp)` transitions so you can answer *"how did this order get here?"*.
- **More real-world states:** add `CancelReplace` / `Expired`, extend the table
  accordingly, and re-check the race test still has exactly-one-winner.
- **Single-atomic snapshot:** transition also carries a side counter (e.g.
  `filled_qty`), and `state() + filled_qty()` must be consistent — pack both
  into one 64-bit atomic so a reader never sees a mismatch.
- **Deadline guard:** reject any transition whose arrival time is older than a
  stored "valid until" timestamp (stale-message prevention).

## Files

- Stub: `src/order_state_machine.cpp`
- Tests: `test/test_order_state_machine.cpp`
- Reference: `SOLUTION.md`