#include "chain_risk.h"

#include <utility>

// TODO(anwer): implement the handlers and chain (see SOLUTION.md).
//
// Suggested shapes:
//   - RiskHandler::handle runs do_check then forwards to next_ (short-circuit
//     on failure); concretes implement only do_check;
//   - MaxNotionalHandler accrues price*qty on passes only;
//   - RiskChain tracks head/tail; append links the tail; validate starts at
//     the head; empty chain accepts.
//
// Stub: handle() always rejects with "not implemented", the notional handler
// never accumulates, and validate() rejects whenever a chain exists -> the
// suite runs RED without hanging or crashing.

void RiskHandler::set_next(std::shared_ptr<RiskHandler> next) {
  next_ = std::move(next);
}

CheckResult RiskHandler::handle(const OrderCheck& /*request*/) {
  return CheckResult{false, "not implemented"};
}

LimitQtyHandler::LimitQtyHandler(std::int64_t max_qty) : max_qty_(max_qty) {}

CheckResult LimitQtyHandler::do_check(const OrderCheck& /*request*/) {
  return CheckResult{true, ""};
}

LimitPriceHandler::LimitPriceHandler(PriceMicros max_price)
    : max_price_(max_price) {}

CheckResult LimitPriceHandler::do_check(const OrderCheck& /*request*/) {
  return CheckResult{true, ""};
}

AllowedSymbolsHandler::AllowedSymbolsHandler(
    std::unordered_set<std::uint64_t> allowed)
    : allowed_(std::move(allowed)) {}

void AllowedSymbolsHandler::allow_symbol(std::uint64_t symbol) {
  allowed_.insert(symbol);
}

CheckResult AllowedSymbolsHandler::do_check(const OrderCheck& /*request*/) {
  return CheckResult{true, ""};
}

MaxNotionalHandler::MaxNotionalHandler(std::int64_t max_notional)
    : max_notional_(max_notional) {}

std::int64_t MaxNotionalHandler::used() const noexcept { return used_; }

void MaxNotionalHandler::reset() noexcept { used_ = 0; }

CheckResult MaxNotionalHandler::do_check(const OrderCheck& /*request*/) {
  return CheckResult{true, ""};
}

void RiskChain::append(std::shared_ptr<RiskHandler> handler) {
  if (!handler) return;
  if (!head_) {
    head_ = handler;
    tail_ = handler;
  } else {
    tail_->set_next(handler);
    tail_ = handler;
  }
}

CheckResult RiskChain::validate(const OrderCheck& /*request*/) {
  return head_ ? CheckResult{false, "not implemented"} : CheckResult{true, ""};
}

std::size_t RiskChain::size() const noexcept {
  std::size_t n = 0;
  for (auto cur = head_; cur; cur = cur->next()) ++n;
  return n;
}

void RiskChain::clear() noexcept {
  head_.reset();
  tail_.reset();
}