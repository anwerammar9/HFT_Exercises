#include "l2_order_book.h"

#include <iterator>
#include <stdexcept>

// Five members to implement. Each carries its own spec below. The HOW is yours.
//
//     cmake --build build -j$(nproc)
//     ctest --test-dir build -L exercise35 --output-on-failure
//
// AN L2 BOOK is aggregated by price: each level carries a total quantity, with
// no individual orders (that is L3 -- exercise 36). Feeds deliver a periodic
// SNAPSHOT of the whole book plus a stream of incremental UPDATES between them,
// and the client must reconcile the two.

// ===========================================================================
// apply_snapshot  --  REPLACE the book wholesale
// ===========================================================================
// POST   the book contains exactly the levels given -- anything previously
//        held is discarded, not merged. A snapshot is the authoritative state.
// ALSO   marks the book as established, which is what lets apply_update start
//        having an effect (see below).
// ASSERTED after a snapshot, best_bid/best_ask report the top of each side
void L2Book::apply_snapshot(std::vector<PriceLevel> bids,
                            std::vector<PriceLevel> asks)
{
  bids_.clear();
  asks_.clear();
  for (auto bid : bids)
  {
    bids_[bid.price] += bid.qty;
  }
  for (auto ask : asks)
  {
    asks_[ask.price] += ask.qty;
  }
  if (!bids.empty() || !asks.empty())
  {
    has_snapshot_ = true;
  }
}

// ===========================================================================
// apply_update  --  apply one incremental level change
// ===========================================================================
// ABSOLUTE, NOT A DELTA  new_qty REPLACES the quantity at that price; it is not
//        added to it.
// NEW PRICE  a price not currently in the book is inserted.
// QTY == 0  DELETES the level. That is how a feed says "this price is gone",
//        and storing a zero-quantity level instead would leave a phantom price
//        at the top of the book.
// BEFORE ANY SNAPSHOT  a silent NO-OP -- not a throw, and not a partial book.
//        Updates routinely arrive before the first snapshot on a live feed,
//        and applying them to nothing would produce a book with holes.
//
// ASSERTED a deeper insert leaves the best price unchanged; an update to an
//            existing level changes its qty (to 250)
//          qty 0 at the best price removes it and REVEALS the next best
//            (bid 99 @700, ask 102)
//          an update with no snapshot yet -> no throw, and both sides still
//            report nullopt
void L2Book::apply_update(Side side, Price price, Qty new_qty)
{
  if (!has_snapshot_)
  {
    return;
  }
  if (side == Side::Bid)
  {
    if (new_qty == 0)
    {
      auto iter = bids_.find(price);
      if (iter != bids_.end())
      {
        bids_.erase(iter);
        return;
      }
    }
    bids_[price] = new_qty;
  }
  if (side == Side::Ask)
  {
    if (new_qty == 0)
    {
      auto iter = asks_.find(price);
      if (iter != asks_.end())
      {
        asks_.erase(iter);
        return;
      }
    }
    asks_[price] = new_qty;
  }
}

// RETURNS  the HIGHEST bid price and its quantity, or nullopt if the bid side
//          is empty. Buyers compete upward, so best means highest.
std::optional<PriceLevel> L2Book::best_bid() const
{
  if (bids_.empty())
    return std::nullopt;
  PriceLevel output;
  output.price = bids_.begin()->first;
  output.qty = bids_.begin()->second;
  return output;
}

// RETURNS  the LOWEST ask price and its quantity, or nullopt if the ask side
//          is empty. Sellers compete downward -- the opposite ordering to bids,
//          which is why the two sides cannot share one comparator.
std::optional<PriceLevel> L2Book::best_ask() const
{
  if (asks_.empty())
    return std::nullopt;
  PriceLevel output;
  output.price = asks_.begin()->first;
  output.qty = asks_.begin()->second;
  return output;
}

// ===========================================================================
// checksum  --  a digest of the whole book, for feed verification
// ===========================================================================
// Exchanges publish a periodic checksum so a client can prove its incrementally
// built book still matches theirs; a mismatch means resubscribe.
//
// DETERMINISTIC  two books built by the same operations must produce the same
//          value -- so it must fold levels in a defined order, not in whatever
//          order a hash container happens to yield.
// SENSITIVE  a book differing in any price or quantity must produce a
//          DIFFERENT value, and a non-empty book must not hash to 0.
// EMPTY BOOK -> 0, by contract.
//
// ASSERTED identical operation sequences -> equal checksums, and non-zero
//          different content -> different checksums
//          an empty book -> 0
std::uint64_t L2Book::checksum() const
{
  std::uint64_t output=0;
  for(auto p : bids_)
  {
    output+=p.first+p.second;
  }
  for(auto p : asks_)
  {
    output+=p.first+p.second;
  }
  return output; // TODO(youssef)
}