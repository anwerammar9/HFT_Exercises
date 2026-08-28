#include "matching_engine.h"

#include <algorithm>
#include <utility>

// TODO(anwer): implement the price-time book (see SOLUTION.md).
//
// Suggested shape: bids_ (std::map<Price, deque<{id,qty}>, greater>) so
// begin() is the best bid; asks_ ascending so begin() is the best ask.
// add_order: sweep the OPPOSITE book best-first, staying on a level until its
// FIFO queue drains, filling at the resting price; leftover qty rests at the
// aggressor price (tail of level). cancel_order: index_ locate + linear remove
// from the level deque, drop empty levels.

std::vector<Trade> MatchingEngine::add_order(Order /*order*/) {
  return {};
}

bool MatchingEngine::cancel_order(OrderId /*id*/) { return false; }