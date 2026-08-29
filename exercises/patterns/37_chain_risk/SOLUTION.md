# Exercise 37 — Chain of Responsibility (Risk Pipeline) (Reference Solution)

**What you implement:** the Chain of Responsibility pattern behind a pre-trade
risk pipeline — handler nodes linked `set_next`-style, a base-class `handle()`
that short-circuits on the first rejection, four concrete handlers, and a
`RiskChain` facade that assembles and runs them.

**Approach**
- `handle()` lives in the base class, **non-virtual**, and *is* the pattern:
  `do_check(request)`; on `!ok` return the result immediately (the chain cuts
  here); otherwise forward to `next_->handle(request)`, or accept (`ok=true`,
  `reason=""`) when there is no `next_`.
- Concrete handlers implement only `do_check`, so each is unit-testable in
  isolation through the public `handle()` and equally chainable.
- `MaxNotionalHandler` is the stateful member: it accrues `price*qty` on a
  pass and rejects *before* the total would exceed the cap (nothing accrues on
  a reject). `used()`/`reset()` expose the state the tests assert.
- `RiskChain` remembers head + tail: `append` hangs a new handler off the
  current tail; `validate` fires from `head_`; `size()` walks the links;
  `clear()` drops both pointers.

## Reference API — `include/chain_risk.h`
#ifndef EXERCISE37_CHAIN_RISK_H_
#define EXERCISE37_CHAIN_RISK_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>

// Chain of Responsibility: pre-trade checks are handler nodes; a request flows
// through them until one rejects (short-circuit) or the chain ends (accept).
//
// Contract:
//   - RiskHandler::handle(request) runs THIS handler's check and, when it
//     passes, forwards to the next handler in the chain. A failing check
//     returns immediately with its reason.
//   - RiskHandler::set_next chains handlers. A handler with no next accepts.
//   - RiskChain is the entry point: append() links handlers in order,
//     validate() runs the chain from the head. An empty chain accepts.
//   - Concrete handlers:
//       LimitQtyHandler(max_qty)          -> rejects qty < 1 or qty > max_qty
//       LimitPriceHandler(max_price)      -> rejects price < 1 or price > max
//       AllowedSymbolsHandler(set)        -> rejects symbols not in the set
//       MaxNotionalHandler(max_notional)  -> STATEFUL: rejects when the
//                                            cumulative price*qty would exceed
//   - reason strings are stable (asserted by tests):
//       "qty limit", "price limit", "symbol not allowed: <sym>",
//       "notional limit".
//   - Stateful placements: a positive price*qty accrues on a PASS; on a REJECT
//     nothing accrues; non-positive inputs accrue nothing.
//
// TODO(anwer): implement (see SOLUTION.md). Stub: handle() always rejects with
// "not implemented" and the notional handler never accumulates -> the chain
// tests run RED.

using PriceMicros = std::int64_t;

enum class Side { kBuy, kSell };

struct OrderCheck {
  std::uint64_t id = 0;
  Side side = Side::kBuy;
  PriceMicros price = 0;
  std::int64_t qty = 0;
  std::uint64_t symbol = 0;
};

struct CheckResult {
  bool ok = false;
  std::string reason;  // "" when ok
};

class RiskHandler {
 public:
  virtual ~RiskHandler() = default;

  void set_next(std::shared_ptr<RiskHandler> next);
  // Non-virtual chaining: run this handler, then forward to `next_`.
  CheckResult handle(const OrderCheck& request);
  std::shared_ptr<RiskHandler> next() const { return next_; }

 protected:
  virtual CheckResult do_check(const OrderCheck& request) = 0;

  std::shared_ptr<RiskHandler> next_;
};

class LimitQtyHandler : public RiskHandler {
 public:
  explicit LimitQtyHandler(std::int64_t max_qty);

 protected:
  CheckResult do_check(const OrderCheck& request) override;

 private:
  std::int64_t max_qty_;
};

class LimitPriceHandler : public RiskHandler {
 public:
  explicit LimitPriceHandler(PriceMicros max_price);

 protected:
  CheckResult do_check(const OrderCheck& request) override;

 private:
  PriceMicros max_price_;
};

class AllowedSymbolsHandler : public RiskHandler {
 public:
  explicit AllowedSymbolsHandler(std::unordered_set<std::uint64_t> allowed);
  void allow_symbol(std::uint64_t symbol);

 protected:
  CheckResult do_check(const OrderCheck& request) override;

 private:
  std::unordered_set<std::uint64_t> allowed_;
};

class MaxNotionalHandler : public RiskHandler {
 public:
  explicit MaxNotionalHandler(std::int64_t max_notional);
  std::int64_t used() const noexcept;
  void reset() noexcept;

 protected:
  CheckResult do_check(const OrderCheck& request) override;

 private:
  std::int64_t max_notional_;
  std::int64_t used_ = 0;
};

class RiskChain {
 public:
  void append(std::shared_ptr<RiskHandler> handler);
  CheckResult validate(const OrderCheck& request);
  std::size_t size() const noexcept;
  void clear() noexcept;

 private:
  std::shared_ptr<RiskHandler> head_;
  std::shared_ptr<RiskHandler> tail_;
};

#endif  // EXERCISE37_CHAIN_RISK_H_

## Reference implementation — `src/chain_risk.cpp`
#include "chain_risk.h"

#include <utility>

void RiskHandler::set_next(std::shared_ptr<RiskHandler> next) {
  next_ = std::move(next);
}

CheckResult RiskHandler::handle(const OrderCheck& request) {
  const CheckResult result = do_check(request);
  if (!result.ok) return result;  // short-circuit: the chain cuts here
  return next_ ? next_->handle(request) : CheckResult{true, ""};
}

LimitQtyHandler::LimitQtyHandler(std::int64_t max_qty) : max_qty_(max_qty) {}

CheckResult LimitQtyHandler::do_check(const OrderCheck& request) {
  if (request.qty < 1 || request.qty > max_qty_) {
    return CheckResult{false, "qty limit"};
  }
  return CheckResult{true, ""};
}

LimitPriceHandler::LimitPriceHandler(PriceMicros max_price)
    : max_price_(max_price) {}

CheckResult LimitPriceHandler::do_check(const OrderCheck& request) {
  if (request.price < 1 || request.price > max_price_) {
    return CheckResult{false, "price limit"};
  }
  return CheckResult{true, ""};
}

AllowedSymbolsHandler::AllowedSymbolsHandler(
    std::unordered_set<std::uint64_t> allowed)
    : allowed_(std::move(allowed)) {}

void AllowedSymbolsHandler::allow_symbol(std::uint64_t symbol) {
  allowed_.insert(symbol);
}

CheckResult AllowedSymbolsHandler::do_check(const OrderCheck& request) {
  if (!allowed_.count(request.symbol)) {
    return CheckResult{false,
                       "symbol not allowed: " + std::to_string(request.symbol)};
  }
  return CheckResult{true, ""};
}

MaxNotionalHandler::MaxNotionalHandler(std::int64_t max_notional)
    : max_notional_(max_notional) {}

std::int64_t MaxNotionalHandler::used() const noexcept { return used_; }

void MaxNotionalHandler::reset() noexcept { used_ = 0; }

CheckResult MaxNotionalHandler::do_check(const OrderCheck& request) {
  const std::int64_t delta = request.price * request.qty;
  if (delta > 0 && used_ + delta > max_notional_) {
    return CheckResult{false, "notional limit"};  // nothing accrues on reject
  }
  if (delta > 0) used_ += delta;
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

CheckResult RiskChain::validate(const OrderCheck& request) {
  return head_ ? head_->handle(request) : CheckResult{true, ""};
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

**How the tests verify you:** the short-circuit test proves handlers *after* a
rejecting one never run (a `RecordingHandler` probe), the pass-through test
proves all handlers run on a clean request, and the two `HandlerOrder...`
chains prove that order decides *which* reason wins for a doubly-bad order.
`MaxNotionalHandler` is the stateful discriminator: accrual on pass, no accrual
on reject, `used()` reset round-trips. A stub `handle()` that always rejects
with "not implemented" keeps everything RED.