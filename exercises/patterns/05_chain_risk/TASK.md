# Exercise patterns/05_chain_risk (ex37) — Chain of Responsibility (Risk Pipeline) (Task)

## The problem (in plain words)

Every order that crosses a risk desk is checked by several independent gates —
max qty, max price, allowed symbols, cumulative notional. The logic itself is
trivial; the *architecture* is the challenge. If the checks live as a giant
`if` statement, adding a new gate means editing the one function and re-testing
everything. The **Chain of Responsibility** pattern makes each check a handler
node and lets the request flow through the chain until one node rejects it — so
a new gate is just a new handler appended at the right position, order is
explicit, and short-circuiting falls out naturally.

## Requirements (what the tests check)

1. `RiskHandler::handle(request)` runs THIS handler's check and, when it
   passes, forwards to the next handler via `set_next`. A failing check returns
   immediately with a stable `reason` string. A handler with no `next_`
   accepts.
2. `RiskChain` is the entry point: `append(handler)` links handlers in order,
   `validate(request)` runs from the head, `size()` counts the chain, `clear()`
   empties it. **An empty chain accepts.**
3. Concrete handlers (stable reasons, asserted exactly):
   - `LimitQtyHandler(max_qty)` → `"qty limit"` (rejects `qty < 1` or `> max`);
   - `LimitPriceHandler(max_price)` → `"price limit"` (rejects `price < 1` or
     `> max`);
   - `AllowedSymbolsHandler(set)` → `"symbol not allowed: <sym>"`; `allow_symbol`
     extends the set;
   - `MaxNotionalHandler(max_notional)` → `"notional limit"` — **stateful**: it
     accrues `price · qty` on accepted checks, rejects before the total would
     exceed the cap (nothing accrues on a reject), with `used()` and `reset()`.
4. **The chain cuts on the first reject** — handlers positioned after a failing
   one are never consulted (a `RecordingHandler` probe proves it).
5. Reordering handlers changes *which* reason a doubly-bad order gets — order
   matters.

## Public API

```cpp
using PriceMicros = std::int64_t;
enum class Side { kBuy, kSell };

struct OrderCheck { std::uint64_t id; Side side; PriceMicros price;
                    std::int64_t qty; std::uint64_t symbol; };
struct CheckResult { bool ok; std::string reason; };

class RiskHandler {              // the Handler interface
 public:
  void set_next(std::shared_ptr<RiskHandler> next);
  CheckResult handle(const OrderCheck& request);    // non-virtual chaining
 protected:
  virtual CheckResult do_check(const OrderCheck& request) = 0;
  std::shared_ptr<RiskHandler> next_;
};

class LimitQtyHandler          : public RiskHandler { /* ... */ };
class LimitPriceHandler        : public RiskHandler { /* ... */ };
class AllowedSymbolsHandler    : public RiskHandler { /* ... */ };
class MaxNotionalHandler       : public RiskHandler { /* ... */ };

class RiskChain {
  void append(std::shared_ptr<RiskHandler> handler);
  CheckResult validate(const OrderCheck& request);
  std::size_t size() const noexcept;
  void clear() noexcept;
};
```

## How to think about it (suggested design)

- `handle()` is the chain algorithm and lives in the base class (non-virtual):
  `do_check(request)` first; on failure return the result (chain cuts); else
  forward to `next_->handle(request)` or accept when `next_` is null.
- Concretes only implement `do_check`. That keeps a handler unit-testable in
  isolation (`h.handle(...)`) **and** chainable (`a->handle(...)` flows into
  `b`).
- `MaxNotionalHandler` keeps `used_` as mutable state across `handle()` calls —
  the reason it is the "stateful" member of the family. Accrue on pass, never
  on reject.
- `RiskChain` just remembers head + tail: `append` links the current tail to
  the new handler; `validate` starts at `head_`.

## Make it harder (optional — not covered by the tests)

- **Async chain:** handlers that are slow (venue reachability, credit backend)
  run on worker pools and the chain returns via continuation; make
  short-circuit propagate across threads.
- **Rules from config:** load handler kinds + parameters from a config file and
  rebuild the chain without recompiling (a mini business-rules engine).
- **Per-request context:** a `RequestContext` threaded through `handle()` that
  handlers read/write (e.g. the accumulated checksum, or a "waive this check"
  flag honoured by later nodes).
- **Transactional notional:** pair each accept with a release path for rejects
  *downstream* of the notional check, so a later failure refunds the accrual.

## Files

- Stub: `src/chain_risk.cpp`
- Tests: `test/test_chain_risk.cpp`
- Reference: `SOLUTION.md`