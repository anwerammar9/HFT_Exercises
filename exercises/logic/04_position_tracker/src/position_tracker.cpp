#include "position_tracker.h"



void PositionTracker::on_fill(Side /*side*/, Qty /*qty*/, Price /*price*/) {}

Qty PositionTracker::position() const noexcept { return 0; }

double PositionTracker::realized_pnl() const noexcept { return 0.0; }

double PositionTracker::unrealized_pnl(Price /*mark_price*/) const noexcept {
  return 0.0;
}

double PositionTracker::avg_cost() const noexcept { return 0.0; }