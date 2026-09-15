#include "strategy_execution.h"

#include <memory>

// TODO(anwer): implement the three algorithms (see SOLUTION.md).
//
// Suggested shapes:
//   - TWAP: base = total/slices + remainder on the last slice (integer math);
//   - VWAP: normalize weights, floor each total*w, then distribute the leftover
//     unit-by-unit to the biggest remainders (ties -> earliest index);
//   - Sniper: one child of the full size at ctx.limit (0 => market).
//   - make_strategy(name): case-insensitive dispatch, invalid_argument unknown.
//
// Stub: run() emits nothing and make_strategy() always returns a TWAP-shaped
// object, so the slicing/registry tests run RED without hanging or crashing.

const char* TwapExecutionStrategy::name() const noexcept { return "twap"; }

void TwapExecutionStrategy::run(const ExecutionContext& /*ctx*/,
                                ExecutionSink /*sink*/) {}

const char* VwapExecutionStrategy::name() const noexcept { return "vwap"; }

void VwapExecutionStrategy::run(const ExecutionContext& /*ctx*/,
                                ExecutionSink /*sink*/) {}

const char* SniperExecutionStrategy::name() const noexcept { return "sniper"; }

void SniperExecutionStrategy::run(const ExecutionContext& /*ctx*/,
                                  ExecutionSink /*sink*/) {}

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

std::unique_ptr<ExecutionStrategy> make_strategy(std::string_view /*name*/) {
  return std::make_unique<TwapExecutionStrategy>();  // stub: always TWAP
}