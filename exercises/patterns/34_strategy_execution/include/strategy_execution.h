#ifndef EXERCISE34_STRATEGY_EXECUTION_H_
#define EXERCISE34_STRATEGY_EXECUTION_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

// Strategy pattern: an execution client swaps between pluggable slicing
// algorithms at runtime without the caller knowing which one is active.
//
// Contract:
//   - ExecutionEngine owns one ExecutionStrategy, `execute()` delegates to it,
//     and `set_strategy()` swaps it live. `strategy_name()` reports the active
//     one.
//   - Every algorithm's run() emits child orders through the ExecutionSink;
//     the child qtys always sum EXACTLY to ctx.total_qty, every child carries
//     ctx.parent_id and ctx.side, and seq is 1-based within the execution.
//   - TwapExecutionStrategy: ctx.slices equal slices, the LAST slice absorbs
//     the remainder. slices == 0 or total_qty <= 0 -> no children.
//   - VwapExecutionStrategy: ctx.weights with largest-remainder rounding;
//     zero-weight buckets emit nothing; empty or all-zero weights -> no
//     children.
//   - SniperExecutionStrategy: exactly one child of the full size, priced at
//     ctx.limit (0 => market print).
//   - make_strategy(name) is the runtime selector by case-insensitive name
//     ("twap" / "vwap" / "sniper"); unknown names throw std::invalid_argument.
//
// A child's `price` of 0 means "market print". Prices are integer micros.
//
// TODO(anwer): implement the algorithms (see SOLUTION.md). Stub: run() emits
// nothing and make_strategy() always returns a TWAP-shaped object, so the
// slicing and registry tests run RED.

using PriceMicros = std::int64_t;

enum class Side { kBuy, kSell };

struct ChildOrder {
  std::int64_t parent_id = 0;
  Side side = Side::kBuy;
  std::int64_t qty = 0;
  PriceMicros price = 0;  // 0 => market print
  std::int64_t seq = 0;   // 1-based within this execution
  bool post_only = false;
};

struct ExecutionContext {
  std::int64_t parent_id = 0;
  Side side = Side::kBuy;
  std::int64_t total_qty = 0;
  PriceMicros limit = 0;        // 0 => children are market prints
  std::size_t slices = 1;       // TWAP: number of child orders
  std::vector<double> weights;  // VWAP: per-slice fractions
  bool post_only = false;       // Sniper: passive print
};

using ExecutionSink = std::function<void(const ChildOrder&)>;

class ExecutionStrategy {
 public:
  virtual ~ExecutionStrategy() = default;
  virtual const char* name() const noexcept = 0;
  virtual void run(const ExecutionContext& ctx, ExecutionSink sink) = 0;
};

class TwapExecutionStrategy final : public ExecutionStrategy {
 public:
  const char* name() const noexcept override;
  void run(const ExecutionContext& ctx, ExecutionSink sink) override;
};

class VwapExecutionStrategy final : public ExecutionStrategy {
 public:
  const char* name() const noexcept override;
  void run(const ExecutionContext& ctx, ExecutionSink sink) override;
};

class SniperExecutionStrategy final : public ExecutionStrategy {
 public:
  const char* name() const noexcept override;
  void run(const ExecutionContext& ctx, ExecutionSink sink) override;
};

class ExecutionEngine {
 public:
  explicit ExecutionEngine(std::unique_ptr<ExecutionStrategy> strategy);
  void set_strategy(std::unique_ptr<ExecutionStrategy> strategy);
  void execute(const ExecutionContext& ctx, ExecutionSink sink);
  const char* strategy_name() const noexcept;

 private:
  std::unique_ptr<ExecutionStrategy> strategy_;
};

// Runtime selector: builds a strategy by case-insensitive name.
// Throws std::invalid_argument for unknown names.
std::unique_ptr<ExecutionStrategy> make_strategy(std::string_view name);

#endif  // EXERCISE34_STRATEGY_EXECUTION_H_