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