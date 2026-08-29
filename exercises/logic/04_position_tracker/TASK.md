# Exercise 04 — Position Tracker (Task)

## The problem (in plain words)

A desk keeps a single **average-cost position** in a symbol: it is long or
short, knows the average entry price (`avg_cost`), and must continuously report
**realized PnL** (locked in on every closing fill) and **unrealized PnL** (what
today's mark price would realize if we closed now). Every fill falls into one
of three cases: it extends the position, it reduces it, or it **flips through
zero** — in which case the old position must be closed completely and a new one
opened at the fill price.

## Requirements (what the tests check)

1. `on_fill(side, qty, price)` updates a **signed** position: `position() > 0`
   long, `position() < 0` short.
2. Same-direction fill averages in:
   `new_avg = (position · avg_cost + qty · price) / (position + qty)`.
3. Closing fill realizes PnL at the old average:
   - long close: `(fill − avg_cost) · qty`
   - short close: `(avg_cost − fill) · qty`
   - a *partial* close keeps the average for the remaining position.
4. A fill that **crosses zero** does two things in one call: fully realize the
   old side, then open the new side at the fill price (new basis — the average
   resets to the fill price and the *remaining* quantity of the crossing fill
   starts the new position).
5. `avg_cost()` = the running average; **0.0 when the position is exactly
   flat**.
6. `realized_pnl()` = cumulative **signed** realized PnL over all closes.
7. `unrealized_pnl(mark)` = `(mark − avg_cost) · position`, correct sign for
   long *and* short; **0.0 when flat**.

### Worked example (verify with this)
Buy 100 @ 50 → position +100, avg 50, realized 0.
Buy 100 @ 60 → position +200, avg 55.
Sell 150 @ 56 → position +50, avg stays 55; realized += (56−55)·150 = 150.
Sell 100 @ 54 → crosses zero: realized += (55−54)·50 = 50 (closes the +50
fully), then opens −50 @ 54; position −50, avg 54.

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

## How to think about it (suggested design)

- Three plain members: `position_`, `avg_cost_`, `realized_pnl_`. No containers
  needed.
- In `on_fill`, map the fill to a **signed delta** (`+qty` buy, `−qty` sell);
  if `delta` is *opposite* to `position_` and larger in magnitude you are in the
  cross-zero case.
- Split the cross-zero fill into close-old + open-new **within one call** and
  keep the ordering exact (realize first, then rebase).
- All arithmetic in `double` for price but `std::int64_t` for quantity; use the
  formulas verbatim.

## Make it harder (optional — not covered by the tests)

- **Per-symbol table:** a `PortfolioTracker` keeping a `PositionTracker` per
  symbol plus a gross daily PnL roll-up.
- **Fees & commission:** subtract `fee(price, qty)` from realized PnL on every
  fill and add a `open_commission` to the basis (mirrors real broker PnL).
- **Alternative costing:** support FIFO and LIFO costing alongside average-cost
  (a batch of `(qty, price)` lots); assert all three reach the same total
  realized PnL.
- **Reporting:** a `pnl_explanation()` that returns a list of per-close
  realized legs so the numbers can be audited.

## Files

- Stub: `src/position_tracker.cpp`
- Tests: `test/test_position_tracker.cpp`
- Reference: `SOLUTION.md`