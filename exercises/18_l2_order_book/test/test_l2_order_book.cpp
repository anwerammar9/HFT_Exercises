#include "l2_order_book.h"

#include <gtest/gtest.h>

namespace {

PriceLevel level(Price p, Qty q) { return PriceLevel{p, q}; }

TEST(L2BookTest, SnapshotEstablishesBook) {
  L2Book book;
  book.apply_snapshot({level(100, 500), level(99, 700), level(98, 300)},
                      {level(101, 200), level(102, 800)});

  auto bb = book.best_bid();  // stub: throws before this -> red
  ASSERT_TRUE(bb.has_value());
  EXPECT_EQ(bb->price, 100);
  EXPECT_EQ(bb->qty, 500);

  auto ba = book.best_ask();
  ASSERT_TRUE(ba.has_value());
  EXPECT_EQ(ba->price, 101);
  EXPECT_EQ(ba->qty, 200);
}

TEST(L2BookTest, UpdateInsertsAndUpdatesLevels) {
  L2Book book;
  book.apply_snapshot({level(100, 500)}, {level(101, 200)});

  book.apply_update(Side::Bid, 99, 300);   // insert a new bid level
  book.apply_update(Side::Ask, 101, 250);  // update the existing ask

  auto bb = book.best_bid();
  ASSERT_TRUE(bb.has_value());
  EXPECT_EQ(bb->price, 100);  // best bid unchanged by the deeper insert

  auto ba = book.best_ask();
  ASSERT_TRUE(ba.has_value());
  EXPECT_EQ(ba->qty, 250);
}

TEST(L2BookTest, ZeroQtyRemovesLevelAndRevealsNextBest) {
  L2Book book;
  book.apply_snapshot({level(100, 500), level(99, 700), level(98, 300)},
                      {level(101, 200), level(102, 800)});

  book.apply_update(Side::Bid, 100, 0);  // remove the best bid
  auto bb = book.best_bid();
  ASSERT_TRUE(bb.has_value());
  EXPECT_EQ(bb->price, 99);
  EXPECT_EQ(bb->qty, 700);

  book.apply_update(Side::Ask, 101, 0);  // remove the best ask
  auto ba = book.best_ask();
  ASSERT_TRUE(ba.has_value());
  EXPECT_EQ(ba->price, 102);
}

TEST(L2BookTest, UpdateBeforeSnapshotIsNoOp) {
  L2Book book;
  EXPECT_NO_THROW(book.apply_update(Side::Bid, 100, 500));  // stub throws -> red
  EXPECT_FALSE(book.best_bid().has_value());
  EXPECT_FALSE(book.best_ask().has_value());
}

TEST(L2BookTest, ChecksumDeterministicAcrossIdenticalOps) {
  auto build = []() {
    L2Book b;
    b.apply_snapshot({level(100, 500), level(99, 300)}, {level(101, 200)});
    b.apply_update(Side::Bid, 100, 700);
    b.apply_update(Side::Ask, 102, 50);
    return b;
  };
  L2Book a = build();
  L2Book c = build();
  EXPECT_EQ(a.checksum(), c.checksum());  // same ops -> same digest
  EXPECT_NE(a.checksum(), 0u);            // stub: 0 -> red

  L2Book empty;
  EXPECT_EQ(empty.checksum(), 0u);  // empty book hashes to 0 (contract)
}

TEST(L2BookTest, ChecksumChangesWithContent) {
  L2Book a;
  a.apply_snapshot({level(100, 500)}, {level(101, 200)});
  L2Book b;
  b.apply_snapshot({level(100, 600)}, {level(101, 200)});  // one qty differs

  EXPECT_NE(a.checksum(), b.checksum());
}

}  // namespace