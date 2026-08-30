# Exercise 34 — Strategy (Execution Selection) (Reference Solution)

**What you implement:** the Strategy pattern behind an execution client — an
`ExecutionEngine` that delegates slicing to a swappable `ExecutionStrategy`,
three concrete algorithms (TWAP / VWAP / Sniper), and a name-based runtime
selector.

**Approach**
- `ExecutionEngine` is deliberately thin: it owns a
  `std::unique_ptr<ExecutionStrategy>` and `execute()` delegates — that thinness
  is the pattern's point (the caller never switches on algorithm type).
- **TWAP:** pure integer math, `base = total / slices` with the remainder on
  the LAST slice (`base + remainder`), so the sum is exact by construction.
  `slices == 0` or `total_qty <= 0` → emit nothing.
- **VWAP:** normalize the weights (sum ≤ 0 → no children), floor each slice
  (`int(total · w)`), accumulate the fractional remainders, then hand the
  leftover one unit at a time to the biggest remainders (ties → earliest
  index) — the same largest-remainder rounding as Exercise 03, so `Σqty`
  equals `total` exactly. Zero-weight buckets are skipped at emit time.
- **Sniper:** exactly one child with the full size, `price = ctx.limit`
  (0 → market print), `post_only = ctx.post_only`.
- `make_strategy(name)`: lowercase-compare against the three fixed names and
  construct the concrete class; anything else throws `std::invalid_argument`.

## Reference API — `include/strategy_execution.h`
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

## Reference implementation — `src/strategy_execution.cpp`
#include "strategy_execution.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {

// Normalized per-slice weight carrying its original index (for tie-breaks).
struct WeightedSlice {
  std::size_t index;
  double remainder;
};

std::vector<double> normalize(const std::vector<double>& weights) {
  const double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
  if (sum <= 0.0) return {};
  std::vector<double> out;
  out.reserve(weights.size());
  for (double w : weights) out.push_back(w / sum);
  return out;
}

}  // namespace

const char* TwapExecutionStrategy::name() const noexcept { return "twap"; }

void TwapExecutionStrategy::run(const ExecutionContext& ctx,
                                ExecutionSink sink) {
  if (ctx.total_qty <= 0 || ctx.slices == 0) return;
  const std::int64_t slices = static_cast<std::int64_t>(ctx.slices);
  const std::int64_t base = ctx.total_qty / slices;
  const std::int64_t remainder = ctx.total_qty % slices;
  for (std::int64_t i = 0; i < slices; ++i) {
    const std::int64_t qty = base + (i + 1 == slices ? remainder : 0);
    sink(ChildOrder{ctx.parent_id, ctx.side, qty, ctx.limit, i + 1, false});
  }
}

const char* VwapExecutionStrategy::name() const noexcept { return "vwap"; }

void VwapExecutionStrategy::run(const ExecutionContext& ctx,
                                ExecutionSink sink) {
  if (ctx.total_qty <= 0 || ctx.weights.empty()) return;
  const std::vector<double> norm = normalize(ctx.weights);
  if (norm.empty()) return;

  // Largest-remainder: floor every slice, then hand the leftover one unit at
  // a time to the slices with the biggest fractional remainders.
  std::vector<std::int64_t> qty(ctx.weights.size(), 0);
  std::vector<WeightedSlice> slices;
  std::int64_t assigned = 0;
  for (std::size_t i = 0; i < ctx.weights.size(); ++i) {
    if (norm[i] <= 0.0) continue;
    const double raw = ctx.total_qty * norm[i];
    const std::int64_t floored = static_cast<std::int64_t>(std::floor(raw));
    qty[i] = floored;
    assigned += floored;
    const double remainder = raw - floored;
    if (remainder > 0.0) slices.push_back(WeightedSlice{i, remainder});
  }
  std::sort(slices.begin(), slices.end(),
            [](const WeightedSlice& a, const WeightedSlice& b) {
              if (a.remainder != b.remainder) return a.remainder > b.remainder;
              return a.index < b.index;  // ties: earliest index first
            });
  std::int64_t leftover = ctx.total_qty - assigned;
  for (const WeightedSlice& s : slices) {
    if (leftover == 0) break;
    qty[s.index] += 1;
    --leftover;
  }

  std::int64_t seq = 0;
  for (std::size_t i = 0; i < ctx.weights.size(); ++i) {
    if (qty[i] == 0) continue;  // zero-weight buckets are skipped
    sink(ChildOrder{ctx.parent_id, ctx.side, qty[i], ctx.limit, ++seq, false});
  }
}

const char* SniperExecutionStrategy::name() const noexcept { return "sniper"; }

void SniperExecutionStrategy::run(const ExecutionContext& ctx,
                                  ExecutionSink sink) {
  if (ctx.total_qty <= 0) return;
  sink(ChildOrder{ctx.parent_id, ctx.side, ctx.total_qty, ctx.limit, 1,
                  ctx.post_only});
}

ExecutionEngine::ExecutionEngine(std::unique_ptr<ExecutionStrategy> strategy)
    : strategy_(std::move(strategy)) {}

void ExecutionEngine::set_strategy(std::unique_ptr<ExecutionStrategy> strategy) {
  strategy_ = std::move(strategy);
}

const char* ExecutionEngine::strategy_name() const noexcept {
  return strategy_ ? strategy_->name() : "";
}

void ExecutionEngine::execute(const ExecutionContext& ctx, ExecutionSink sink) {
  if (strategy_) strategy_->run(ctx, sink);
}

std::unique_ptr<ExecutionStrategy> make_strategy(std::string_view name) {
  // Compare case-insensitively against the fixed algorithm names.
  std::string lower;
  lower.reserve(name.size());
  for (char c : name) lower.push_back(static_cast<char>(std::tolower(c)));
  if (lower == "twap") return std::make_unique<TwapExecutionStrategy>();
  if (lower == "vwap") return std::make_unique<VwapExecutionStrategy>();
  if (lower == "sniper") return std::make_unique<SniperExecutionStrategy>();
  throw std::invalid_argument("unknown execution strategy: " +
                              std::string(name));
}

**How the tests verify you:** the twap/vwap/sniper tests assert exact child
qtys and sums (the largest-remainder and last-slice-remainder are the
discriminators), the registry test asserts that `make_strategy` returns the
*right* concrete strategy for mixed-case names, and the engine test proves
`set_strategy()` really swaps behavior. Slicing stubs emit nothing and the
selector always hands back a TWAP, keeping the suite RED.