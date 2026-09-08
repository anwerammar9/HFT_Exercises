#include "order_gateway.h"

#include <chrono>

// TODO(anwer): implement the real gateway (see SOLUTION.md).
//
// Suggested shape:
//   submit(cid, order):
//     now = now_();  lock mu_;
//     compact_locked(now)  // erase entries where now - ts > window
//     if (seen maps cid AND now - ts <= window) return false;
//     seen_[cid] = now; sink_(order); (forward under the lock) return true;
//   compact_locked: erase ids whose stamp is older than dedup_window_.

namespace {
std::uint64_t to_ns(std::chrono::steady_clock::time_point t) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          t.time_since_epoch())
          .count());
}
}  // namespace

std::uint64_t OrderGateway::DefaultClock() {
  return to_ns(std::chrono::steady_clock::now());
}

OrderGateway::OrderGateway(std::chrono::nanoseconds dedup_window, Sink sink,
                           NowFn now)
    : dedup_window_(dedup_window), sink_(std::move(sink)),
      now_(std::move(now)) {}

bool OrderGateway::submit(ClientOrderId /*cid*/, const Order& /*order*/) {
  return false;  // stub: never forwards
}