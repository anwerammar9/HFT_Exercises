#include "observer_market_data.h"

// TODO(anwer): implement the feed (see SOLUTION.md).
//
// Suggested shapes:
//   - a std::vector<Slot> {id, {observer, symbol}} behind one mutex; ids are a
//     monotonic counter starting at 1;
//   - publish_quote/publish_trade: copy the slot vector under the lock, run
//     callbacks on the copy OUTSIDE the lock (filter by symbol, try/catch and
//     unsubscribe() the slot on throw).
//
// Stub: subscribe returns the invalid id 0, so nothing is ever registered,
// unsubscribe is false, the count stays 0 and publishes are no-ops -> all the
// fan-out tests run RED without hanging or crashing.

ObserverId MarketDataFeed::subscribe(std::shared_ptr<MarketDataObserver> /*observer*/,
                                     SymbolId /*symbol*/) {
  return 0;
}

bool MarketDataFeed::unsubscribe(ObserverId /*id*/) { return false; }

std::size_t MarketDataFeed::subscriber_count() const { return 0; }

void MarketDataFeed::publish_quote(const Quote& /*quote*/) {}

void MarketDataFeed::publish_trade(const Trade& /*trade*/) {}