# Exercise 36 — Command (Order-Entry Log) (Task)

## The problem (in plain words)

An order-entry path does three things continuously: **new**, **modify** and
**cancel**. If each of these is a plain function call scattered across the code,
then "remember what happened" and "take it back" require special casing
everywhere. The **Command** pattern wraps each action in an object that knows
how to *execute itself* and *undo itself*; the invoker just runs them and keeps
a log — so undo is free (last-in-first-out), and the history is exactly the log
of what ran.

## Requirements (what the tests check)

1. `OrderRepository` is the receiver: `add` (fails on duplicate id), `modify`
   (active orders only), `cancel` (active orders only), `remove` (erase
   entirely), `view(id)` → `OrderView`, `size()`.
2. `OrderCommand` subclasses implement `type()`, `order_id()`, `execute()`,
   `undo()`, `describe()`:
   - **NewOrderCommand:** undo = remove the order entirely (as if it never
     existed);
   - **ModifyOrderCommand:** execute snapshots the prior price/qty; undo restores
     exactly those;
   - **CancelOrderCommand:** execute snapshots the live fields; undo re-adds the
     order with the exact same fields (active again).
3. `ExecuteCommandProcessor` (the invoker) keeps a **LIFO log**:
   - `submit(cmd)` executes then appends **only on success** (returns success);
   - `undo_last()` reverses the most recent logged command and pops it (returns
     `false` when the log is empty);
   - `log_size()` counts the logged (successful) commands.
4. `undo()` on a command that never successfully executed is a **safe no-op**.
5. `describe()` is deterministic and asserted exactly.

## Public API

```cpp
using PriceMicros = std::int64_t;
enum class Side { kBuy, kSell };
enum class OrderStatus { kNone, kActive, kCancelled };
enum class CommandType { kNew, kModify, kCancel };

struct OrderView { bool present; Side side; PriceMicros price;
                   std::int64_t qty; OrderStatus status; };

class OrderRepository {          // receiver
  bool add(std::uint64_t id, Side side, PriceMicros price, std::int64_t qty);
  bool modify(std::uint64_t id, PriceMicros new_price, std::int64_t new_qty);
  bool cancel(std::uint64_t id);
  bool remove(std::uint64_t id);
  OrderView view(std::uint64_t id) const;
  std::size_t size() const;
};

class OrderCommand {             // the Command interface
  virtual CommandType type() const noexcept = 0;
  virtual std::uint64_t order_id() const noexcept = 0;
  virtual bool execute() = 0;
  virtual void undo() = 0;
  virtual std::string describe() const = 0;
};
class NewOrderCommand final : public OrderCommand { /* ... */ };
class ModifyOrderCommand final : public OrderCommand { /* ... */ };
class CancelOrderCommand final : public OrderCommand { /* ... */ };

class ExecuteCommandProcessor {  // the invoker + log
  bool submit(std::unique_ptr<OrderCommand> command);
  bool undo_last();
  std::size_t log_size() const;
};
```

## How to think about it (suggested design)

- The repository is a plain `std::unordered_map<uint64_t, Entry>`; entries carry
  `status` so `modify`/`cancel` refuse to touch a `kCancelled` order.
- Each command holds an `applied_` flag set on *successful* execute. `undo()`
  checks it first → that's what makes undo-before-execute a safe no-op and
  prevents double-undo effects.
- `ModifyOrderCommand` and `CancelOrderCommand` snapshot their prior state
  *inside `execute()`*, from `repo_.view(id_)`, because they cannot know it
  until they run. This is the subtle bit the tests probe (undo through a
  modify → cancel chain restores the exact original fields).
- The invoker moves the `unique_ptr` into `log_` only after `execute()`
  returned true — a failed command is never logged, so `undo_last()` only ever
  reverses applied work.

## Make it harder (optional — not covered by the tests)

- **Replay:** add a `replay()` that re-executes the whole log against a fresh
  repository — the standard way to rebuild state after a failover.
- **Redo:** keep a second "undone" stack and re-apply commands with the same
  stored snapshot (undo/redo, editor-style).
- **Task batching:** `submit_all({new, modify, cancel})` that executes as one
  transactional unit — either every command applies and is logged, or none.
- **Threaded invoker:** a lock around the log so `submit` can be called from
  an execution thread while a state thread walks the log.

## Files

- Stub: `src/command_order_entry.cpp`
- Tests: `test/test_command_order_entry.cpp`
- Reference: `SOLUTION.md`