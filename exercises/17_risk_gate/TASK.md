# Exercise 17 — Pre-Trade Risk Gate (Task)

## Problem
An atomic position/notional limit checker. The naive
`if (would_exceed_limit()) return false; commit();` is a **check-then-act
race** under concurrent callers and must be solved **without a coarse lock**.

## Requirements (what the tests check)
1. `check_and_reserve(order)` succeeds iff BOTH limits still hold **after** the
   order were applied:
   - position: `|position + delta| ≤ max_position`, where
     `delta = +qty` for Buy, `−qty` for Sell;
   - notional: `notional_used + price·qty ≤ max_notional`, where notional is
     **GROSS** (buys and sells both accrue).
2. On success the counters are committed and `true` is returned; otherwise
   **nothing changes** and `false` is returned (a reject reserves nothing).
3. `release(order)` returns reserved capacity (safe to call idempotently).
4. `position()` / `notional_used()` report the committed values (exact, not
   approximate).
5. Concurrency: N threads each reserving `limit/N` prove the committed total
   can **never** exceed the limit (TSan-clean; runs under `tsan;stress`).

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

## Design notes
Pack BOTH counters into the one provided 64-bit atomic (`notional_used` in the
upper 32 bits, signed `position` in the lower 32) and do a single CAS loop:
load → compute candidate → check both limits → CAS-commit. Optimistic retry,
no lock. Position/notional must fit in `int32`.

## Files
- Stub: `src/risk_gate.cpp`
- Tests: `test/test_risk_gate.cpp`
- Reference: `SOLUTION.md`