#ifndef EXERCISE27_SEMAPHORE_H_
#define EXERCISE27_SEMAPHORE_H_

#include <condition_variable>
#include <cstddef>
#include <mutex>

// A counting semaphore built from std::mutex + std::condition_variable.
//
// A semaphore is a budget counter, not a "one-thread-inside" lock: any thread
// may release() to produce a token, and any acquire() consumes one, blocking
// while the budget is empty. This is the primitive that throttles a fan-in
// (N producers wait on one semaphore holding K tokens) or gates a pooled
// resource with a fixed number of units.
//
// Contract:
//   - acquire(): consumes one token, blocking (with std::condition_variable,
//     so spurious wakeups are handled naturally) until one is available.
//   - try_acquire(): consumes one token if (and only if) one is immediately
//     available; never blocks, never consumes on failure.
//   - release(): produces one token and wakes a single waiter. Callable by any
//     thread, not only the one that acquired — that is the whole point.
//   - count(): the current token count. A racy snapshot by nature — only
//     meaningful between operations (it takes the lock so it's TSan-clean).
//
// TODO(anwer): implement in src/semaphore.cpp.
//   - Guard `n_` with `mu_`.
//     * acquire():   std::unique_lock lock(mu_); cv_.wait(lock, n_ > 0); --n_;
//     * try_acquire(): std::lock_guard lock(mu_); if (n_ == 0) return false;
//       --n_; return true;
//     * release():   std::lock_guard lock(mu_); ++n_; cv_.notify_one();
//     * count():     std::lock_guard lock(mu_); return n_;
//   - Construction: explicit (one-arg) so `Semaphore s;` doesn't silently
//     compile; pass the initial token count explicitly.

class Semaphore {
 public:
  explicit Semaphore(std::size_t initial = 0);

  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  void acquire();
  bool try_acquire();
  void release();
  std::size_t count() const;  // snapshot; locks mu_ to stay TSan-clean

 private:
  mutable std::mutex mu_;  // mutable: count() is const but takes the lock
  std::condition_variable cv_;
  std::size_t n_;
};

#endif  // EXERCISE27_SEMAPHORE_H_