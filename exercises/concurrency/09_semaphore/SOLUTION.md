# Exercise concurrency/09_semaphore (ex27) — Counting Semaphore (Reference Solution)

**What you implement:** a counting semaphore (mutex + condition variable) whose
three properties the tests enforce: `acquire()` blocks while empty,
`release()` wakes exactly one waiter, and at most `K` threads are ever inside
the guarded section for a budget of `K`.

**Approach**
- Shared state: `n_` tokens guarded by `mu_`, with `cv_` for blocking.
- `acquire()`: `unique_lock(mu_)`, then `cv_.wait(lock, n_ > 0)` — the
  predicate is mandatory so spurious wakeups loop back to waiting instead of
  consuming an (absent) token — then `--n_`.
- `try_acquire()`: single lock; `false` when `n_ == 0`, else `--n_` and `true`.
  Failure never consumes.
- `release()`: `++n_` under the lock, `notify_one()` *after* unlocking — a
  woken waiter can run immediately instead of re-contending on our lock. Works
  from any thread; tokens are a shared budget, not thread-owned.
- `count()`: returns `n_` under the lock (TSan-clean; a snapshot regardless).
- No `std::counting_semaphore` used — that's the exercise.

## Reference API — `include/counting_semaphore.h`

```cpp
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
```

## Reference implementation — `src/semaphore.cpp`

```cpp
#include "counting_semaphore.h"

#include <mutex>

Semaphore::Semaphore(std::size_t initial) : n_(initial) {}

void Semaphore::acquire() {
  std::unique_lock<std::mutex> lock(mu_);
  cv_.wait(lock, [this] { return n_ > 0; });
  --n_;
}

bool Semaphore::try_acquire() {
  std::lock_guard<std::mutex> lock(mu_);
  if (n_ == 0) return false;
  --n_;
  return true;
}

void Semaphore::release() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    ++n_;
  }
  cv_.notify_one();
}

std::size_t Semaphore::count() const {
  std::lock_guard<std::mutex> lock(mu_);
  return n_;
}
```

**How the tests verify you:** `EmptySemaphoreTryAcquireFailsWithoutConsuming` and
the blocking test pin the acquire/release contract; `AtMostLimitThreadsHoldSimultaneously`
runs 16 threads against a budget of 4 and fails if more than 4 enter at once —
the no-op-stub discriminator. All tests must stay green under TSan (`build-tsan`).