# Exercise 04 — Position Tracker (Task)

## Problem
Track an average-cost position and realized/unrealized PnL: same-direction
fills average into the holding, closing fills realize PnL at the moving
average, and a fill that crosses zero must split into "close the old side" +
"open a new position at the fill price".

## Requirements (what the tests check)
1. `on_fill(side, qty, price)` maintains a **signed** position (`position()`:
   long +, short −).
2. Same-side fill averages in: `new_avg = (pos·avg + qty·price) / (pos+qty)`.
3. Closing fill realizes PnL:
   - long close: `(fill − avg_cost) · qty`
   - short close: `(avg_cost − fill) · qty`
   - partial close keeps the average.
4. Fill that **crosses zero**: realize the old side fully, then open the new
   side at the fill price (new basis).
5. `avg_cost()` = running average; **0 when flat** (exactly zero position).
6. `realized_pnl()` = cumulative signed realized PnL.
7. `unrealized_pnl(mark)` = `(mark − avg_cost) · position`, signed and correct
   for long *and* short; **0 when flat**.

## Public API
```cpp
using Qty = std::int64_t;
using Price = double;
enum class Side { Buy, Sell };

class PositionTracker {
  void on_fill(Side side, Qty qty, Price price);
  Qty position() const noexcept;
  double realized_pnl() const noexcept;
  double unrealized_pnl(Price mark_price) const noexcept;
  double avg_cost() const noexcept;
};
```

## Files
- Stub: `src/position_tracker.cpp`
- Tests: `test/test_position_tracker.cpp`
- Reference: `SOLUTION.md`