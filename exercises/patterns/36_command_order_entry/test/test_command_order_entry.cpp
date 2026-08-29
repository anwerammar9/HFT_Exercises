#include "command_order_entry.h"

#include <memory>

#include <gtest/gtest.h>

namespace {

constexpr PriceMicros kP10 = 1'000'000;
constexpr PriceMicros kP20 = 2'000'000;

TEST(CommandOrderEntryTest, NewCommandPlacesOrderAndLogs) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  EXPECT_TRUE(proc.submit(  // stub: submit false -> red
      std::make_unique<NewOrderCommand>(repo, 7, Side::kBuy, kP10, 50)));
  auto v = repo.view(7);
  EXPECT_TRUE(v.present);  // stub: nothing registered -> red
  EXPECT_EQ(v.qty, 50);
  EXPECT_EQ(v.status, OrderStatus::kActive);
  EXPECT_EQ(repo.size(), 1u);
  EXPECT_EQ(proc.log_size(), 1u);
}

TEST(CommandOrderEntryTest, UndoNewRemovesOrder) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  proc.submit(std::make_unique<NewOrderCommand>(repo, 3, Side::kSell, kP10, 10));
  ASSERT_TRUE(proc.undo_last());  // stub: undo_last false -> red
  EXPECT_FALSE(repo.view(3).present);
  EXPECT_EQ(repo.size(), 0u);
  EXPECT_EQ(proc.log_size(), 0u);
}

TEST(CommandOrderEntryTest, ModifyUndoRestoresPriorState) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  proc.submit(std::make_unique<NewOrderCommand>(repo, 7, Side::kBuy, kP10, 100));
  EXPECT_TRUE(proc.submit(  // stub: submit false -> red
      std::make_unique<ModifyOrderCommand>(repo, 7, kP20, 40)));
  EXPECT_EQ(repo.view(7).qty, 40);
  EXPECT_EQ(repo.view(7).price, kP20);

  ASSERT_TRUE(proc.undo_last());  // stub: undo_last false -> red
  EXPECT_EQ(repo.view(7).qty, 100);
  EXPECT_EQ(repo.view(7).price, kP10);
  EXPECT_EQ(repo.view(7).status, OrderStatus::kActive);
  EXPECT_EQ(proc.log_size(), 1u);  // only the New remains logged
}

TEST(CommandOrderEntryTest, CancelUndoReactivatesOrder) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  proc.submit(std::make_unique<NewOrderCommand>(repo, 7, Side::kBuy, kP10, 100));
  EXPECT_TRUE(proc.submit(  // stub: submit false -> red
      std::make_unique<CancelOrderCommand>(repo, 7)));
  EXPECT_EQ(repo.view(7).status, OrderStatus::kCancelled);

  ASSERT_TRUE(proc.undo_last());  // stub: undo_last false -> red
  auto v = repo.view(7);
  EXPECT_TRUE(v.present);
  EXPECT_EQ(v.status, OrderStatus::kActive);
  EXPECT_EQ(v.qty, 100);
  EXPECT_EQ(v.price, kP10);
}

TEST(CommandOrderEntryTest, LifoUndoChainReturnsToEmpty) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  proc.submit(std::make_unique<NewOrderCommand>(repo, 1, Side::kBuy, kP10, 10));
  proc.submit(std::make_unique<ModifyOrderCommand>(repo, 1, kP20, 5));
  ASSERT_TRUE(proc.undo_last());  // undo the modify
  ASSERT_TRUE(proc.undo_last());  // undo the new
  EXPECT_FALSE(repo.view(1).present);
  EXPECT_EQ(proc.log_size(), 0u);
}

TEST(CommandOrderEntryTest, FailedCommandsAreNotLogged) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  // cancel of an unknown order must fail and never enter the log
  EXPECT_FALSE(proc.submit(std::make_unique<CancelOrderCommand>(repo, 99)));
  EXPECT_EQ(proc.log_size(), 0u);

  // duplicate New must fail and not be logged (the first one stays)
  EXPECT_TRUE(proc.submit(  // stub: submit false -> red
      std::make_unique<NewOrderCommand>(repo, 5, Side::kBuy, kP10, 9)));
  EXPECT_FALSE(proc.submit(std::make_unique<NewOrderCommand>(repo, 5, Side::kSell, kP10, 9)));
  EXPECT_EQ(proc.log_size(), 1u);

  // modify of a cancelled order fails
  proc.submit(std::make_unique<CancelOrderCommand>(repo, 5));
  EXPECT_FALSE(proc.submit(std::make_unique<ModifyOrderCommand>(repo, 5, kP20, 3)));
  EXPECT_EQ(proc.log_size(), 2u);
}

TEST(CommandOrderEntryTest, UndoBeforeExecuteIsNoopAndUndoEmptyLogFails) {
  OrderRepository repo;
  NewOrderCommand cmd(repo, 8, Side::kBuy, kP10, 1);
  cmd.undo();  // never executed -> safe no-op
  EXPECT_FALSE(repo.view(8).present);

  ExecuteCommandProcessor proc;
  EXPECT_FALSE(proc.undo_last());
}

TEST(CommandOrderEntryTest, DescribeIsDeterministic) {
  OrderRepository repo;
  NewOrderCommand n(repo, 7, Side::kBuy, kP10, 50);
  ModifyOrderCommand m(repo, 7, kP20, 30);
  CancelOrderCommand c(repo, 7);
  EXPECT_EQ(n.describe(), "new id=7 side=Buy price=1000000 qty=50");
  EXPECT_EQ(m.describe(), "modify id=7 price=2000000 qty=30");
  EXPECT_EQ(c.describe(), "cancel id=7");
}

TEST(CommandOrderEntryTest, UndoRestoresThroughModifyAndCancelChain) {
  OrderRepository repo;
  ExecuteCommandProcessor proc;
  proc.submit(std::make_unique<NewOrderCommand>(repo, 4, Side::kSell, kP10, 20));
  proc.submit(std::make_unique<ModifyOrderCommand>(repo, 4, kP20, 6));
  proc.submit(std::make_unique<CancelOrderCommand>(repo, 4));

  ASSERT_TRUE(proc.undo_last());  // undo cancel -> active at the modified fields
  {
    auto v = repo.view(4);
    EXPECT_EQ(v.status, OrderStatus::kActive);
    EXPECT_EQ(v.qty, 6);
    EXPECT_EQ(v.price, kP20);
  }
  ASSERT_TRUE(proc.undo_last());  // undo modify -> back to the original fields
  {
    auto v = repo.view(4);
    EXPECT_EQ(v.status, OrderStatus::kActive);
    EXPECT_EQ(v.qty, 20);
    EXPECT_EQ(v.price, kP10);
  }
}

}  // namespace