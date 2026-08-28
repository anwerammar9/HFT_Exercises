# Exercise 10 — Timer Wheel (Task)

## Problem
A single-threaded, **tick-driven** timer (matching-engine event-loop style):
the driving thread owns the wheel and advances time explicitly. A timer fires
exactly once; callbacks may re-entrantly schedule/cancel; the wheel wraps
correctly for delays longer than one full rotation.

## Requirements (what the tests check)
1. Time advances **only** via `tick(now)` (ms, monotonic; internal clock starts
   at 0ms).
2. `schedule(delay, cb)` fires `cb` **exactly once** at the first `tick(now)`
   where `now >= (wheel time when scheduled) + delay`. Never fires early.
3. Returns a unique, opaque `TimerId`; **0 is never valid**.
4. `cancel(id)` returns `true` iff the timer was still pending (callback
   prevented); `false` (and safe) for already-fired/unknown ids; idempotent.
5. A callback running inside `tick()` may call `schedule()`/`cancel()`
   re-entrantly; a new timer's deadline is computed from the wheel's time *at
   that moment* — it fires on a later tick, never during the tick that ran its
   parent.
6. Wrap-around: `delay > 256ms` (more than one rotation) still fires at the
   correct absolute tick.

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

## Design notes
1ms granularity, `kWheelSize` slots per rotation. Classic bug to avoid: timers
whose delay crosses a rotation landing at the wrong absolute tick (give each
timer a rotation counter so it lands many rotations ahead). The private members
and a min-heap are already reserved in the header for your scheme.

## Files
- Stub: `src/timer_wheel.cpp`
- Tests: `test/test_timer_wheel.cpp`
- Reference: `SOLUTION.md`