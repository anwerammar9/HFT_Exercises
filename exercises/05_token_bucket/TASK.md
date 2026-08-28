# Exercise 05 — Token Bucket Rate Limiter (Task)

## Problem
A token bucket for order throttling with **lazy (on-demand) refill** — no refill
thread or timer on the hot path — and an **injectable logical clock** so tests
never sleep.

## Requirements (what the tests check)
1. Constructed from `rate_per_sec` (tokens/sec) and `burst` quantized to token
   capacity.
2. Bucket **starts full** (`burst` tokens available immediately).
3. `try_acquire(tokens)`: consumes `tokens` and returns `true` iff they are
   available; otherwise returns `false` **without consuming anything**. Never
   blocks.
4. `advance(elapsed)` is the ONLY way time moves. The next `try_acquire`
   refills `tokens += elapsed · rate` since the last refill, **capped at
   `burst`**.
5. Thread-safe: concurrent callers can never overspend the budget (guard
   `{tokens, last_refill, now}` with the provided mutex).

## Public API
```cpp
class TokenBucket {
  explicit TokenBucket(double rate_per_sec, double burst);
  bool try_acquire(double tokens = 1.0);
  void advance(std::chrono::nanoseconds elapsed);
};
```

## Files
- Stub: `src/token_bucket.cpp`
- Tests: `test/test_token_bucket.cpp`
- Reference: `SOLUTION.md`