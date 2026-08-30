#include "command_order_entry.h"

#include <memory>

// TODO(anwer): implement the receiver, commands and invoker (see SOLUTION.md).
//
// Suggested shapes:
//   - OrderRepository: unordered_map<id, {side, price, qty, status}> behind
//     add/modify/cancel/remove/view;
//   - each command tracks `applied_` set on a successful execute(); undo()
//     guards on it; Modify/Cancel snapshot prior state from repo_.view(id_)
//     inside execute();
//   - ExecuteCommandProcessor::submit executes then logs only on success;
//     undo_last() pops and undoes the newest command.
//
// Stub: everything always fails (repo.add/modify/cancel/remove false, view()
// not-present, execute false, submit false, undo_last false, log empty) so the
// suite runs RED without hanging or crashing.

bool OrderRepository::add(std::uint64_t /*id*/, Side /*side*/,
                          PriceMicros /*price*/, std::int64_t /*qty*/) {
  return false;
}

bool OrderRepository::modify(std::uint64_t /*id*/, PriceMicros /*price*/,
                             std::int64_t /*qty*/) {
  return false;
}

bool OrderRepository::cancel(std::uint64_t /*id*/) { return false; }

bool OrderRepository::remove(std::uint64_t /*id*/) { return false; }

OrderView OrderRepository::view(std::uint64_t /*id*/) const { return OrderView{}; }

std::size_t OrderRepository::size() const { return 0; }

NewOrderCommand::NewOrderCommand(OrderRepository& repo, std::uint64_t id,
                                 Side side, PriceMicros price, std::int64_t qty)
    : repo_(repo), id_(id), side_(side), price_(price), qty_(qty) {}

bool NewOrderCommand::execute() { return false; }

void NewOrderCommand::undo() {}

std::string NewOrderCommand::describe() const { return ""; }

ModifyOrderCommand::ModifyOrderCommand(OrderRepository& repo, std::uint64_t id,
                                       PriceMicros price, std::int64_t qty)
    : repo_(repo), id_(id), new_price_(price), new_qty_(qty) {}

bool ModifyOrderCommand::execute() { return false; }

void ModifyOrderCommand::undo() {}

std::string ModifyOrderCommand::describe() const { return ""; }

CancelOrderCommand::CancelOrderCommand(OrderRepository& repo, std::uint64_t id)
    : repo_(repo), id_(id) {}

bool CancelOrderCommand::execute() { return false; }

void CancelOrderCommand::undo() {}

std::string CancelOrderCommand::describe() const { return ""; }

bool ExecuteCommandProcessor::submit(std::unique_ptr<OrderCommand> /*command*/) {
  return false;
}

bool ExecuteCommandProcessor::undo_last() { return false; }

std::size_t ExecuteCommandProcessor::log_size() const { return 0; }