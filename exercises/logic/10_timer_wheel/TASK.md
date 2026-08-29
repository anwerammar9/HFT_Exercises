# Exercise 10 — Timer Wheel (Task)

## The problem (in plain words)

A matching engine or market-data loop is a **single thread that owns time**: it
wakes each millisecond and calls `tick(now)`. Timers must fire **exactly
once**, at or after their deadline, and a callback may re-entrantly schedule or
cancel other timers while a tick is running. The classic trap: a delay longer
than one full wheel rotation must still land at the correct absolute tick
(wrap-around).

## Requirements (what the tests check)

1. Time advances **only** via `tick(now)` — a monotonically increasing
   milliseconds value. The wheel keeps an internal clock that starts at 0ms.
2. `schedule(delay, cb)` fires `cb` **exactly once**, at the first `tick(now)`
   where `now >= (wheel time when scheduled) + delay`. Never fires early.
3. Returns a unique, opaque `TimerId`; **0 is never a valid id**.
4. `cancel(id)` returns `true` iff the timer was still pending (its callback
   was prevented); `false` — and safe — for already-fired or unknown ids;
   idempotent (cancelling twice is fine).
5. Re-entrancy: a callback running inside `tick()` may call `schedule()` /
   `cancel()`. A timer scheduled from inside a callback is computed against the
   wheel's time *at that moment* and therefore fires on a **later** tick, never
   during the tick that ran its parent.
6. **Wrap-around:** a `delay > 256ms` (more than one rotation) still fires at
   the correct absolute tick.

## Public API

```cpp
class TimerWheel {
  using TimerId = std::uint64_t;
  static constexpr std::uint64_t kWheelSize = 256;  // 1ms slots

  TimerId schedule(std::chrono::milliseconds delay, std::function<void()> cb);
  bool cancel(TimerId id);
  void tick(std::chrono::milliseconds now);
};
```

## How to think about it (suggested design)

- Granularity is 1ms per slot; `kWheelSize = 256` slots make one rotation.
- Give every timer an **absolute deadline** and a **rotation counter**, so a
  delay of, say, 700ms can be represented even though the wheel only spans
  256ms. The private members in the header already reserve the shape: a
  min-heap (`pq_`) keyed by deadline, `pending_`/`done_` sets, `now_`,
  `next_id_`.
- `schedule()`: compute `deadline = now_ + delay`, assign a fresh `id`/`seq`,
  push into the heap, mark pending.
- `tick(now)`: set `now_ = now`, then pop every heap entry whose deadline has
  passed, run its callback **in order**, and move the id from `pending_` to
  `done_` *before* invoking the callback (so re-entrant `cancel(id)` inside the
  callback returns `false`).
- Because the heap is keyed by absolute deadline, wrap-around is handled
  automatically — you never need to count rotations by hand.

## Make it harder (optional — not covered by the tests)

- **Periodic timers:** `schedule_repeating(delay, cb)` that re-schedules itself
  at `deadline + delay`; guard each firing against drift.
- **Timers with payloads:** store a `void*`/callback argument alongside the id.
- **Wheel-with-day-scale:** a hierarchical wheel (second wheel of 60 nodes on
  top of the ms wheel) so a single wheel stays small — the classic kafka/DPDK
  design.
- **Stats & safety:** expose `pending_count()`, `missed_tick_count()`, and a
  guard that asserts `tick(now)` never goes *backwards*.

## Files

- Stub: `src/timer_wheel.cpp`
- Tests: `test/test_timer_wheel.cpp`
- Reference: `SOLUTION.md`