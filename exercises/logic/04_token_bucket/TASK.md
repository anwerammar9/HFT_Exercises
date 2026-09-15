# Exercise logic/04_token_bucket (ex05) — Token Bucket Rate Limiter (Task)

## The problem (in plain words)

Order flow must be throttled: at most `rate_per_sec` orders per second on
average, but with the freedom to catch up in short bursts, capped at `burst`.
The classic token bucket does this. This exercise adds the two production
constraints: **no refill thread or timer** (refilling is computed lazily on the
next request), and an **injectable logical clock** so the tests never actually
sleep.

## Requirements (what the tests check)

1. Constructed from `rate_per_sec` (tokens per second) and `burst` (the
   maximum number of tokens the bucket can hold = the burst size).
2. The bucket **starts full**: `burst` tokens are available immediately.
3. `try_acquire(tokens)`:
   - returns `true` and **consumes** `tokens` when they are available;
   - returns `false` **without consuming anything** when not;
   - never blocks, never spins.
4. `advance(elapsed)` is the **only** way time moves. It pushes the internal
   logical clock forward by `elapsed`; the *next* `try_acquire` refills
   `tokens += elapsed · rate` measured since the last refill, **capped at
   `burst`** (unused time never accumulates beyond burst — no saving up).
5. Thread-safe: concurrent callers can never overspend the budget. Guard
   `{tokens, last_refill, now}` with the provided mutex.

> The mocking contract matters: with a rate of `1.0` and one `advance(1s)`,
> `try_acquire()` must succeed exactly **one** time — rounding must not leak
> fractions of a token.

## Public API

```cpp
class TokenBucket {
  explicit TokenBucket(double rate_per_sec, double burst);
  bool try_acquire(double tokens = 1.0);
  void advance(std::chrono::nanoseconds elapsed);
};
```

## How to think about it (suggested design)

- Store `tokens_`, `rate_per_sec_`, `burst_`, and two clock values
  (`now_` and `last_refill_`, both nanoseconds).
- On each `try_acquire`: compute the elapsed time since `last_refill_`,
  refill `tokens_ = min(burst_, tokens_ + elapsed · rate)`, update
  `last_refill_`, then decide if `tokens >= requested`.
- Keep everything under the one mutex (or later a CAS loop) — a plain
  `std::mutex` is the correct, boring default.
- Single source of time: only `advance()` moves `now_`.

## Make it harder (optional — not covered by the tests)

- **Two-stage shape:** “sustained rate + burst” as *two* buckets in series
  (the classic token frame) and test the combined envelope.
- **Per-caller limits:** a table `{caller → TokenBucket}` plus a global budget,
  so one heavy user can't starve the pool.
- **Peek:** a non-consuming `tokens_available()` and a `reset()`-to-full.
- **Blocking acquire:** a third method that blocks with a deadline, purely on a
  `condition_variable`, so threads can wait instead of dropping.

## Files

- Stub: `src/token_bucket.cpp`
- Tests: `test/test_token_bucket.cpp`
- Reference: `SOLUTION.md`