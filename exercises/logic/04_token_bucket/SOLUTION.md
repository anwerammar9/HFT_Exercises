# Exercise logic/04_token_bucket (ex05) — Token Bucket Rate Limiter (Reference Solution)

**What you implement:** a token bucket for order throttling with *lazy/on-demand
refill* — no refill thread or timer on the hot path — plus an injectable
logical clock so tests are deterministic (no real `sleep()`).

**Approach**
- **Lazy refill:** `try_acquire` computes
  `elapsed = now_ - last_refill_`, top-ups `tokens_ = min(burst_, tokens_ +
  elapsed·rate)` and stamps `last_refill_ = now_`. Failed acquisitions still
  bank the partial token (nothing is consumed unless it fits) — that's what
  the *exactly-one-per-interval* test relies on.
- **Logical clock:** `advance(elapsed)` is the only way `now_` moves; tests
  drive elapsed time deterministically. No `sleep()` anywhere.
- **Thread-safety:** one small `std::mutex` around {tokens, last_refill, now}
  guarantees concurrent callers can never overspend (the TSan budget test).
- Bucket starts **full** (`tokens_ = burst` in the ctor) per contract.

## Reference API — `include/token_bucket.h`
#ifndef EXERCISE05_TOKEN_BUCKET_H_
#define EXERCISE05_TOKEN_BUCKET_H_

#include <chrono>
#include <mutex>

// Token bucket rate limiter (order throttling) with lazy refill and an
// injectable logical clock.
//
// Contract:
//   - Constructed with `rate_per_sec` tokens/sec and `burst` capacity; the
//     bucket STARTS FULL (burst tokens immediately available).
//   - try_acquire(tokens): consumes `tokens` if available and returns true;
//     otherwise returns false without consuming anything. Never blocks.
//   - advance(elapsed) is the ONLY way time moves in the tests (no real
//     sleep). It appends `elapsed` to an internal virtual clock; the next
//     try_acquire refills by the time elapsed since the last refill, CAPPED
//     at `burst`.
//
// Implementation notes (reference solution):
//   - No refill thread / timer on the hot path (the low-latency pattern):
//     elapsed = now - last_refill; refill = min(burst, tokens + elapsed * rate).
//   - Guard {tokens, last_refill, now} with a small mutex (or a CAS loop) so
//     concurrent callers can never overspend the budget.
//
// TODO(anwer): implement the bucket (see SOLUTION.md). Stub: try_acquire is
// always false and advance is a no-op -> every budget test runs RED.
class TokenBucket {
 public:
  explicit TokenBucket(double rate_per_sec, double burst);

  bool try_acquire(double tokens = 1.0);
  void advance(std::chrono::nanoseconds elapsed);

 private:
  double rate_per_sec_{0.0};
  double burst_{0.0};
  double tokens_{0.0};
  std::chrono::nanoseconds last_refill_{0};
  std::chrono::nanoseconds now_{0};
  std::mutex mu_;
};

#endif  // EXERCISE05_TOKEN_BUCKET_H_

## Reference implementation — `src/token_bucket.cpp`
#include "token_bucket.h"

#include <algorithm>
#include <mutex>

// Lazy refill, no refill thread or timer on the hot path (the low-latency
// pattern): on each try_acquire we top up by the time elapsed since the last
// refill, capped at `burst`. Time only moves via advance() (the injectable
// logical clock the tests drive). One small mutex keeps concurrent callers
// from overspending the budget.

TokenBucket::TokenBucket(double rate_per_sec, double burst)
    : rate_per_sec_(rate_per_sec), burst_(burst), tokens_(burst) {}

bool TokenBucket::try_acquire(double tokens) {
  std::lock_guard<std::mutex> g(mu_);

  const double elapsed_ns =
      static_cast<double>((now_ - last_refill_).count());
  const double refill = elapsed_ns * rate_per_sec_ / 1e9;
  tokens_ = std::min(burst_, tokens_ + refill);
  last_refill_ = now_;

  if (tokens_ < tokens) return false;
  tokens_ -= tokens;
  return true;
}

void TokenBucket::advance(std::chrono::nanoseconds elapsed) {
  std::lock_guard<std::mutex> g(mu_);
  now_ += elapsed;
}