# Exercise 34 — Strategy (Execution Selection) (Task)

## The problem (in plain words)

The same execution client must slice a large parent order differently depending
on the algorithm in charge today: **TWAP** (equal time slices), **VWAP** (slices
weighted by expected volume), or **Sniper** (one aggressive single print). The
**Strategy** pattern lets you swap the algorithm at runtime — `execute()` always
looks identical from the caller's side, and the active policy is injected, not
hardcoded. The hard part everyone forgets: the slice sizes must sum to
**exactly** the parent size, no matter which algorithm runs.

## Requirements (what the tests check)

1. `ExecutionEngine` owns one `ExecutionStrategy`; `execute(ctx, sink)` feeds
   every emitted child order to the sink in order; `set_strategy()` swaps the
   algorithm live; `strategy_name()` reports which one is active.
2. Every strategy must emit children whose qtys sum **exactly** to
   `ctx.total_qty`, each carrying `ctx.parent_id`, `ctx.side`, a 1-based `seq`.
3. **TwapExecutionStrategy:** `ctx.slices` equal slices, the **last** slice
   absorbs the remainder (`base + remainder`). `slices == 0` or `total_qty <= 0`
   → no children.
4. **VwapExecutionStrategy:** slices weighted by `ctx.weights`, rounded with
   **largest remainder** so the sum stays exact; zero-weight buckets emit
   nothing; empty or all-zero weights → no children.
5. **SniperExecutionStrategy:** exactly one child of the full size, priced at
   `ctx.limit` (`0` → market print).
6. `make_strategy(name)` selects by **case-insensitive** name
   (`"twap"`/`"vwap"`/`"sniper"`); unknown names throw `std::invalid_argument`.

## Public API

```cpp
using PriceMicros = std::int64_t;
enum class Side { kBuy, kSell };

struct ChildOrder {
  std::int64_t parent_id; Side side; std::int64_t qty;
  PriceMicros price;      // 0 => market print
  std::int64_t seq; bool post_only;
};

struct ExecutionContext {
  std::int64_t parent_id; Side side; std::int64_t total_qty;
  PriceMicros limit;                 // 0 => children are market prints
  std::size_t slices;                // TWAP
  std::vector<double> weights;       // VWAP
  bool post_only;                    // Sniper
};

using ExecutionSink = std::function<void(const ChildOrder&)>;

class ExecutionStrategy {
  virtual const char* name() const noexcept = 0;
  virtual void run(const ExecutionContext& ctx, ExecutionSink sink) = 0;
};
class TwapExecutionStrategy final : public ExecutionStrategy { /* ... */ };
class VwapExecutionStrategy final : public ExecutionStrategy { /* ... */ };
class SniperExecutionStrategy final : public ExecutionStrategy { /* ... */ };

class ExecutionEngine {
  void set_strategy(std::unique_ptr<ExecutionStrategy> strategy);
  void execute(const ExecutionContext& ctx, ExecutionSink sink);
  const char* strategy_name() const noexcept;
};

std::unique_ptr<ExecutionStrategy> make_strategy(std::string_view name);
```

## How to think about it (suggested design)

- Reuse the exact-rounding maths from Exercise 03: TWAP is `base = total /
  slices` with the remainder on the last slice (pure integer arithmetic);
  VWAP normalizes the weights, floors each `total · w`, then gives the leftover
  one unit at a time to the biggest fractional remainders (ties → earliest
  index).
- The engine is deliberately thin: it holds a `unique_ptr<ExecutionStrategy>`
  and `execute()` just delegates. That thinness is the Strategy pattern's whole
  point — the caller swaps algorithms without rewriting anything.
- `make_strategy` lower-cases the name and constructs the concrete class;
  it's the runtime selector that makes the pattern testable end-to-end.

## Make it harder (optional — not covered by the tests)

- **Benchmark-driven autoswitch:** track each strategy's fill rate / slippage
  and have the engine swap to the best performer mid-parent (a real
  "adaptive execution" client).
- **Template Method hybrid:** factor the "parent tracking → slice → ack"
  skeleton into an abstract base with hooks, keeping slicing as the pluggable
  piece.
- **Iceberg:** a new `IcebergExecutionStrategy` that shows only a `display_qty`
  slice at a time and re-prints as fills arrive — add it without touching the
  engine or existing strategies.
- **Safety caps:** engine-level post-condition checks that the emitted sum
  matched `total_qty` and log a panic otherwise.

## Files

- Stub: `src/strategy_execution.cpp`
- Tests: `test/test_strategy_execution.cpp`
- Reference: `SOLUTION.md`