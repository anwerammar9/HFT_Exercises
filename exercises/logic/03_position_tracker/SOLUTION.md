# Exercise logic/03_position_tracker (ex04) — Position Tracker (Reference Solution)

**What you implement:** an average-cost position and PnL tracker — buys/sells
on the same side average into one book, closes realize PnL at the moving
average, and a fill that *crosses zero* splits cleanly into "close the old
side" + "open the new side".

**Approach**
- **Opening fill** (flat book): `position = ±qty`, `avg_cost = fill price`.
- **Same direction**: standard weighted average
  `new_avg = (position·avg + qty·price) / (position + qty)`.
- **Opposite direction**: realize `min(|position|, |qty|)` first —
  long close: `(fill − avg)·qty`, short close: `(avg − fill)·qty` — then
  apply the residual.
  - Partial close (sign unchanged): average cost untouched.
  - Crossed zero (sign flipped): the remaining qty opens a new position **at
    the fill price** — the old basis is realized, a fresh one established.
  - Exactly flat: `avg_cost` resets to **0** (the chosen contract).
- `unrealized_pnl(mark) = (mark − avg)·position` is signed and correct for
  both sides; 0 when flat. `realized_pnl()` accumulates every close.

## Reference API — `include/position_tracker.h`
#ifndef EXERCISE04_POSITION_TRACKER_H_
#define EXERCISE04_POSITION_TRACKER_H_

#include <cstdint>

// Average-cost position & realized/unrealized PnL tracker.
//
// Contract:
//   - on_fill updates an average-cost position. A fill in the same direction
//     averages into the position; a closing fill realizes PnL; a fill that
//     CROSSES ZERO splits into "close the old position" (realize PnL) + "open
//     a new position at the fill price" (resets the basis for the new side).
//   - position(): signed (long +, short -).
//   - realized_pnl(): cumulative signed realized PnL from closes:
//         long close:  (fill - avg_cost) * qty;   short close: (avg_cost - fill) * qty.
//   - unrealized_pnl(mark): (mark - avg_cost) * position — signed, correct for
//     both long and short; 0 when flat.
//   - avg_cost(): 0 when flat (chosen contract); the running average otherwise.
//
// TODO(anwer): implement the tracker (see SOLUTION.md). Stub: on_fill is a
// no-op and every read returns 0 -> tests run RED.
using Qty = std::int64_t;
using Price = double;

enum class Side { Buy, Sell };

class PositionTracker {
 public:
  void on_fill(Side side, Qty qty, Price price);

  Qty position() const noexcept;
  double realized_pnl() const noexcept;
  double unrealized_pnl(Price mark_price) const noexcept;
  double avg_cost() const noexcept;

 private:
  Qty position_{0};
  double avg_cost_{0.0};
  double realized_pnl_{0.0};
};

#endif  // EXERCISE04_POSITION_TRACKER_H_

## Reference implementation — `src/position_tracker.cpp`
#include "position_tracker.h"

#include <algorithm>
#include <cstdlib>

// Average-cost position. Same-direction fills AVERAGE IN; closing fills
// realize PnL at the moving average; a fill that crosses zero splits into
// "close the old position, realize PnL" + "open the new one at the fill
// price". avg_cost_ resets to 0 when the book goes flat.

void PositionTracker::on_fill(Side side, Qty qty, Price price) {
  if (qty == 0) return;

  const Qty signed_qty = side == Side::Buy ? qty : -qty;

  if (position_ == 0) {  // opening fill establishes the basis
    position_ = signed_qty;
    avg_cost_ = price;
    return;
  }

  if ((position_ > 0) == (signed_qty > 0)) {  // same direction: average in
    const double total_cost = position_ * avg_cost_ + signed_qty * price;
    position_ += signed_qty;
    avg_cost_ = total_cost / static_cast<double>(position_);
    return;
  }

  // Opposite direction: close some/all of the position first.
  const Qty old_pos = position_;
  const Qty closing = std::min(std::llabs(old_pos), std::llabs(signed_qty));
  if (old_pos > 0)
    realized_pnl_ += (price - avg_cost_) * closing;  // long close
  else
    realized_pnl_ += (avg_cost_ - price) * closing;  // short close

  position_ += signed_qty;

  if (position_ == 0) {
    avg_cost_ = 0.0;  // flat: basis reset (chosen contract)
  } else if ((position_ > 0) != (old_pos > 0)) {
    avg_cost_ = price;  // crossed zero: new position opens at the fill price
  }
  // else partial close: average cost is unchanged.
}

Qty PositionTracker::position() const noexcept { return position_; }

double PositionTracker::realized_pnl() const noexcept { return realized_pnl_; }

double PositionTracker::unrealized_pnl(Price mark_price) const noexcept {
  if (position_ == 0) return 0.0;
  return (mark_price - avg_cost_) * position_;
}

double PositionTracker::avg_cost() const noexcept { return avg_cost_; }