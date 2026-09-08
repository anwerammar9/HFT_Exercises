# Exercise patterns/04_command_order_entry (ex36) — Command (Order-Entry Log) (Reference Solution)

**What you implement:** the Command pattern behind an order-entry path — every
action (new / modify / cancel) is an object that can execute *and undo itself*,
and the invoker runs commands through a LIFO log, so undo is free and the
history is exactly the set of commands that succeeded.

**Approach**
- `OrderRepository` is a plain `unordered_map<uint64_t, Entry>` where `Entry`
  carries `status`. `add` fails on a duplicate id; `modify`/`cancel` fail when
  the order is not `kActive`; `remove` erases entirely (undo-of-New).
- Each command keeps an `applied_` flag set only on a *successful* `execute()`;
  `undo()` returns immediately unless it is set — that single flag is what
  makes undo-before-execute a safe no-op and double-undo a no-op too.
- `ModifyOrderCommand` and `CancelOrderCommand` snapshot their prior state
  inside `execute()` from `repo_.view(id_)` (they cannot know it earlier):
  modify restores price/qty; cancel remembers the live side/price/qty and
  re-adds them on undo. This is the state the chain test exercises.
- The invoker appends to `log_` only after `execute()` returned true, so a
  failed command (cancel of an unknown id, duplicate new, modify of a
  cancelled order) never enters the log and `undo_last()` only reverses applied
  work. `undo_last()` pops the newest entry → LIFO semantics.

## Reference API — `include/command_order_entry.h`
#ifndef EXERCISE36_COMMAND_ORDER_ENTRY_H_
#define EXERCISE36_COMMAND_ORDER_ENTRY_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Command pattern: every action on an order (new / modify / cancel) is a
// Command object that ExecuteCommandProcessor can run, log and undo.
//
// Contract:
//   - OrderRepository is the receiver: add()/modify()/cancel()/remove() plus
//     view() give single-threaded order state. add() fails when the id is
//     already present; modify()/cancel() fail when the order is not ACTIVE;
//     remove() erases an entry entirely (used to undo a New).
//   - Commands are polymorphic: type(), order_id(), execute(), undo(),
//     describe().
//       NewOrderCommand    : execute = repo.add(...); undo = remove the order
//                            entirely (back to "never existed").
//       ModifyOrderCommand : execute = repo.modify(...); undo = restore the
//                            price/qty it had BEFORE this execute.
//       CancelOrderCommand : execute = repo.cancel(...); undo = re-add the
//                            order with the exact fields it had when LIVE.
//   - ExecuteCommandProcessor is the invoker with a LIFO log:
//       submit(cmd) executes then logs ONLY on success (returns success);
//       undo_last() reverses the most recent logged command and pops it
//       (false when the log is empty).
//   - undo() on a command that never successfully executed is a safe no-op.
//
// TODO(anwer): implement (see SOLUTION.md). Stub: repo operations always fail,
// commands never execute (return false), nothing is logged and undo_last()
// returns false -> the suites run RED.

using PriceMicros = std::int64_t;

enum class Side { kBuy, kSell };
enum class OrderStatus { kNone, kActive, kCancelled };
enum class CommandType { kNew, kModify, kCancel };

struct OrderView {
  bool present = false;
  Side side = Side::kBuy;
  PriceMicros price = 0;
  std::int64_t qty = 0;
  OrderStatus status = OrderStatus::kNone;
};

class OrderRepository {
 public:
  bool add(std::uint64_t id, Side side, PriceMicros price, std::int64_t qty);
  bool modify(std::uint64_t id, PriceMicros new_price, std::int64_t new_qty);
  bool cancel(std::uint64_t id);
  bool remove(std::uint64_t id);
  OrderView view(std::uint64_t id) const;
  std::size_t size() const;

 private:
  struct Entry {
    Side side = Side::kBuy;
    PriceMicros price = 0;
    std::int64_t qty = 0;
    OrderStatus status = OrderStatus::kNone;
  };
  std::unordered_map<std::uint64_t, Entry> orders_;
};

class OrderCommand {
 public:
  virtual ~OrderCommand() = default;
  virtual CommandType type() const noexcept = 0;
  virtual std::uint64_t order_id() const noexcept = 0;
  virtual bool execute() = 0;
  virtual void undo() = 0;
  virtual std::string describe() const = 0;
};

class NewOrderCommand final : public OrderCommand {
 public:
  NewOrderCommand(OrderRepository& repo, std::uint64_t id, Side side,
                  PriceMicros price, std::int64_t qty);
  CommandType type() const noexcept override { return CommandType::kNew; }
  std::uint64_t order_id() const noexcept override { return id_; }
  bool execute() override;
  void undo() override;
  std::string describe() const override;

 private:
  OrderRepository& repo_;
  std::uint64_t id_;
  Side side_;
  PriceMicros price_;
  std::int64_t qty_;
  bool applied_ = false;
};

class ModifyOrderCommand final : public OrderCommand {
 public:
  ModifyOrderCommand(OrderRepository& repo, std::uint64_t id,
                     PriceMicros new_price, std::int64_t new_qty);
  CommandType type() const noexcept override { return CommandType::kModify; }
  std::uint64_t order_id() const noexcept override { return id_; }
  bool execute() override;
  void undo() override;
  std::string describe() const override;

 private:
  OrderRepository& repo_;
  std::uint64_t id_;
  PriceMicros new_price_;
  std::int64_t new_qty_;
  bool applied_ = false;
  PriceMicros old_price_ = 0;
  std::int64_t old_qty_ = 0;
};

class CancelOrderCommand final : public OrderCommand {
 public:
  explicit CancelOrderCommand(OrderRepository& repo, std::uint64_t id);
  CommandType type() const noexcept override { return CommandType::kCancel; }
  std::uint64_t order_id() const noexcept override { return id_; }
  bool execute() override;
  void undo() override;
  std::string describe() const override;

 private:
  OrderRepository& repo_;
  std::uint64_t id_;
  bool applied_ = false;
  Side old_side_ = Side::kBuy;
  PriceMicros old_price_ = 0;
  std::int64_t old_qty_ = 0;
};

class ExecuteCommandProcessor {
 public:
  bool submit(std::unique_ptr<OrderCommand> command);
  bool undo_last();
  std::size_t log_size() const;

 private:
  std::vector<std::unique_ptr<OrderCommand>> log_;
};

#endif  // EXERCISE36_COMMAND_ORDER_ENTRY_H_

## Reference implementation — `src/command_order_entry.cpp`
#include "command_order_entry.h"

#include <utility>

// ---- Receiver ---------------------------------------------------------------

bool OrderRepository::add(std::uint64_t id, Side side, PriceMicros price,
                          std::int64_t qty) {
  if (orders_.count(id)) return false;
  orders_.emplace(id, Entry{side, price, qty, OrderStatus::kActive});
  return true;
}

bool OrderRepository::modify(std::uint64_t id, PriceMicros new_price,
                             std::int64_t new_qty) {
  auto it = orders_.find(id);
  if (it == orders_.end() || it->second.status != OrderStatus::kActive)
    return false;
  it->second.price = new_price;
  it->second.qty = new_qty;
  return true;
}

bool OrderRepository::cancel(std::uint64_t id) {
  auto it = orders_.find(id);
  if (it == orders_.end() || it->second.status != OrderStatus::kActive)
    return false;
  it->second.status = OrderStatus::kCancelled;
  return true;
}

bool OrderRepository::remove(std::uint64_t id) { return orders_.erase(id) > 0; }

OrderView OrderRepository::view(std::uint64_t id) const {
  auto it = orders_.find(id);
  if (it == orders_.end()) return OrderView{};
  OrderView v;
  v.present = true;
  v.side = it->second.side;
  v.price = it->second.price;
  v.qty = it->second.qty;
  v.status = it->second.status;
  return v;
}

std::size_t OrderRepository::size() const { return orders_.size(); }

// ---- Commands ---------------------------------------------------------------

NewOrderCommand::NewOrderCommand(OrderRepository& repo, std::uint64_t id,
                                 Side side, PriceMicros price, std::int64_t qty)
    : repo_(repo), id_(id), side_(side), price_(price), qty_(qty) {}

bool NewOrderCommand::execute() {
  if (!repo_.add(id_, side_, price_, qty_)) return false;
  applied_ = true;
  return true;
}

void NewOrderCommand::undo() {
  if (!applied_) return;
  repo_.remove(id_);  // back to "never existed"
  applied_ = false;
}

std::string NewOrderCommand::describe() const {
  std::string s = "new id=" + std::to_string(id_);
  s += " side=";
  s += (side_ == Side::kBuy) ? "Buy" : "Sell";
  s += " price=" + std::to_string(price_);
  s += " qty=" + std::to_string(qty_);
  return s;
}

ModifyOrderCommand::ModifyOrderCommand(OrderRepository& repo, std::uint64_t id,
                                       PriceMicros new_price,
                                       std::int64_t new_qty)
    : repo_(repo), id_(id), new_price_(new_price), new_qty_(new_qty) {}

bool ModifyOrderCommand::execute() {
  const OrderView before = repo_.view(id_);
  if (!before.present || before.status != OrderStatus::kActive) return false;
  old_price_ = before.price;
  old_qty_ = before.qty;
  if (!repo_.modify(id_, new_price_, new_qty_)) return false;
  applied_ = true;
  return true;
}

void ModifyOrderCommand::undo() {
  if (!applied_) return;
  if (repo_.view(id_).present) repo_.modify(id_, old_price_, old_qty_);
  applied_ = false;
}

std::string ModifyOrderCommand::describe() const {
  std::string s = "modify id=" + std::to_string(id_);
  s += " price=" + std::to_string(new_price_);
  s += " qty=" + std::to_string(new_qty_);
  return s;
}

CancelOrderCommand::CancelOrderCommand(OrderRepository& repo, std::uint64_t id)
    : repo_(repo), id_(id) {}

bool CancelOrderCommand::execute() {
  const OrderView before = repo_.view(id_);
  if (!before.present || before.status != OrderStatus::kActive) return false;
  old_side_ = before.side;
  old_price_ = before.price;
  old_qty_ = before.qty;
  if (!repo_.cancel(id_)) return false;
  applied_ = true;
  return true;
}

void CancelOrderCommand::undo() {
  if (!applied_) return;
  // Re-add the order with the exact LIVE fields it had when cancelled.
  repo_.remove(id_);  // in case of an odd transition, add() must not collide
  repo_.add(id_, old_side_, old_price_, old_qty_);
  applied_ = false;
}

std::string CancelOrderCommand::describe() const {
  return "cancel id=" + std::to_string(id_);
}

// ---- Invoker ----------------------------------------------------------------

bool ExecuteCommandProcessor::submit(std::unique_ptr<OrderCommand> command) {
  if (!command || !command->execute()) return false;
  log_.push_back(std::move(command));
  return true;
}

bool ExecuteCommandProcessor::undo_last() {
  if (log_.empty()) return false;
  log_.back()->undo();
  log_.pop_back();
  return true;
}

std::size_t ExecuteCommandProcessor::log_size() const { return log_.size(); }

**How the tests verify you:** the execute/undo tests assert the repository
state after each command and after each `undo_last()` — a stub that never logs
and never executes leaves everything red. The chain test (`UndoRestoresThroughModifyAndCancelChain`)
is the discriminator for the LIFO + snapshot logic: undo(cancel) then undo(modify)
must land back on the *original* fields. The "failed commands not logged" test
checks that rejections never pollute the log.