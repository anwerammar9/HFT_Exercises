# Exercise 17 — Pre-Trade Risk Gate (Task)

## The problem (in plain words)

Before an order goes to the exchange, risk checks **position** and **gross
notional** limits: `|position + delta| ≤ max_position` and
`notional_used + price·qty ≤ max_notional`. The trap is that many threads check
and commit at the same time — naive
`if (would_exceed_limit()) return false; commit();` lets two callers both pass
the limit. You must make check-and-commit **atomic on the counters themselves**,
optimistically, **with no coarse lock** anywhere on the hot path.

## Requirements (what the tests check)

1. `check_and_reserve(order)` succeeds **iff both limits still hold after** the
   order were applied:
   - position: `|position + delta| ≤ max_position`, where `delta = +qty` for
     Buy and `−qty` for Sell;
   - notional: `notional_used + price·qty ≤ max_notional`, where notional is
     **GROSS** — buys *and* sells both accrue.
2. On success the counters are committed and `true` returned; otherwise
   **nothing changes** and `false` is returned (a reject reserves nothing).
3. `release(order)` returns reserved capacity (order rejected or cancelled
   downstream). It must be safe to call — the pool stays consistent.
4. `position()` / `notional_used()` report the **committed** values exactly,
   never approximate.
5. **Concurrency:** N threads each reserving `limit/N` prove the committed
   total can **never** exceed the limit — TSan-clean, runs under `tsan;stress`.

## Public API

```cpp
using Qty = std::int64_t;
using Price = std::int64_t;
using Notional = std::int64_t;
enum class Side { Buy, Sell };
struct Order { std::int64_t id; Side side; Price price; Qty qty; };

class RiskGate {
  RiskGate(Qty max_position, Notional max_notional);
  bool check_and_reserve(const Order& order) noexcept;
  void release(const Order& order) noexcept;
  Qty position() const noexcept;
  Notional notional_used() const noexcept;
};
```

## How to think about it (suggested design)

- The header already reserves the trick: pack **both** counters into the single
  64-bit atomic `state_` — `notional_used` in the upper 32 bits, signed
  `position` in the lower 32. (Position/notional must fit in `int32`.)
- `check_and_reserve` is one **CAS retry loop**:
  1. `cur = state_.load()`; decode position + notional;
  2. compute the candidate (position + delta, notional + price·qty);
  3. if either limit would be exceeded → `return false`;
  4. `compare_exchange(cur, packed_candidate)` → success on hit, else loop and
     re-read (someone else moved the counters; retry with the fresh values).
- `release` is the same loop with the candidate moving in the *opposite*
  direction.
- No mutex, no coarse lock — just the single atomic and an optimistic loop.

## Make it harder (optional — not covered by the tests)

- **Per-symbol limits:** extend to `{symbol → (max_position, max_notional)}`
  with a global overlay, keeping per-symbol state in its own packed atomic
  (or a small table behind a shared mutex, and argue the tradeoff).
- **Soft vs hard limits:** two thresholds — reserve "soft" capacity that can be
  overshot with a flag, hard never.
- **Double-spend guard:** track `order id → reserved (position, notional)` so a
  buggy caller that releases the same order twice can be detected (instead of
  silently inflating the pool).
- **Daily reset + time window:** a paired `start_of_day` counter scheme, and a
  `check_only(order)` query that answers without committing.
- **Grow the packing:** move to a `std::atomic<std::uint64_t>` + 32-bit-notional
  overflow *detection* instead of silently assuming it fits — and test the
  boundary.

## Files

- Stub: `src/risk_gate.cpp`
- Tests: `test/test_risk_gate.cpp`
- Reference: `SOLUTION.md`