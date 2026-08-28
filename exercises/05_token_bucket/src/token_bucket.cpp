#include "token_bucket.h"

// TODO(anwer): implement the real bucket (see SOLUTION.md).
//
// Suggested shape: under `mu_` (or a CAS loop over {tokens, last_refill}) —
//   advance(e): now_ += e;   (logical clock; the ONLY time source in tests)
//   try_acquire(t): grown = min(burst_,
//                   tokens_ + (now_ - last_refill_) * rate_per_sec_);
//                   if (grown >= t) { tokens_ = grown - t; return true; }
//                   else { tokens_ = grown; return false; }
//                   and always set last_refill_ = now_.

TokenBucket::TokenBucket(double rate_per_sec, double burst)
    : rate_per_sec_(rate_per_sec), burst_(burst) {}

bool TokenBucket::try_acquire(double /*tokens*/) {
  return false;  // stub: the bucket is always empty
}

void TokenBucket::advance(std::chrono::nanoseconds /*elapsed*/) {}