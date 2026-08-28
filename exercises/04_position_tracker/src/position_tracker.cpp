#include "position_tracker.h"

// TODO(anwer): implement the tracker (see SOLUTION.md).
//
// Suggested shape (per the contract):
//   on_fill: position==0   -> position = +/-qty, avg_cost = price;
//            same sign    -> average in: avg = (pos*avg + qty*price)/(pos+qty);
//            opposite     -> realize close min(|pos|,|qty|) first:
//                              long:  + (price-avg)*close;
//                              short: + (avg-price)*close;
//                            then apply the residual; if the FLIP or go flat,
//                            avg_cost = flat? 0 : price (basis resets).
//   Avg cost stays put on a partial close. Reads are trivial.

void PositionTracker::on_fill(Side /*side*/, Qty /*qty*/, Price /*price*/) {}

Qty PositionTracker::position() const noexcept { return 0; }

double PositionTracker::realized_pnl() const noexcept { return 0.0; }

double PositionTracker::unrealized_pnl(Price /*mark_price*/) const noexcept {
  return 0.0;
}

double PositionTracker::avg_cost() const noexcept { return 0.0; }