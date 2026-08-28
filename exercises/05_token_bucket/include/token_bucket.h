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