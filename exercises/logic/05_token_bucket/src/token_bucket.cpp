#include "token_bucket.h"


TokenBucket::TokenBucket(double rate_per_sec, double burst)
    : rate_per_sec_(rate_per_sec), burst_(burst) {}

bool TokenBucket::try_acquire(double /*tokens*/) {
  return false;  // stub: the bucket is always empty
}

void TokenBucket::advance(std::chrono::nanoseconds /*elapsed*/) {}