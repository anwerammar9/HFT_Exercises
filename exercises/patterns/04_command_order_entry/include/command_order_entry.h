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