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