# Exercise 27 — Counting Semaphore (Task)

## The problem (in plain words)

A **counting semaphore** is a budget counter with tokens: `acquire()` spends
one (blocking while empty), `release()` produces one — and — unlike a mutex —
*any* thread can `release()`, you don't need to be the thread that `acquire`d.
It is the primitive you reach for to gate a fixed pool of resources (DB
connections, seats, units) or to throttle a fan-in with an N-at-a-time budget.

## Requirements (what the tests check)

1. `Semaphore(initial)` starts with that many tokens available.
2. `acquire()` consumes one token; on an empty semaphore it **blocks** until
   some thread `release()`es (handles spurious wakeups correctly).
3. `try_acquire()` consumes one token **iff** one is immediately available —
   never blocks, and on failure it must NOT consume anything.
4. `release()` adds one token and wakes a single waiter. Releases accumulate:
   5 releases → a budget of 5.
5. **The discriminator:** a semaphore with budget `K` must never let more than
   `K` threads be inside the guarded section at once, no matter how many
   threads try. (A no-op or missing acquire fails this test red.)
6. `count()` returns a snapshot of the current balance (it locks internally so
   it's race-free to *call*; the value is still inherently racy).
7. No `std::counting_semaphore` / `std::binary_semaphore` — build it by hand.

## Public API

```cpp
class Semaphore {
  explicit Semaphore(std::size_t initial = 0);
  void acquire();
  bool try_acquire();
  void release();
  std::size_t count() const;
 private:
  std::mutex mu_;
  std::condition_variable cv_;
  std::size_t n_;
};
```

## How to think about it (suggested design)

- One counter `n_`, guarded by a `std::mutex`, notified by a
  `std::condition_variable`.
  - `acquire()`: `{ unique_lock(mu_); cv_.wait(lock, [&]{ return n_ > 0; }); --n_; }`
    — the predicate makes spurious wakeups a non-issue.
  - `try_acquire()`: one lock, check/`--n_`/return or `false`.
  - `release()`: `{ lock_guard(mu_); ++n_; } cv_.notify_one();` — notify
    *outside* the lock so a newly-awoken waiter doesn't immediately block on
    your mutex again.
- Why the mutex+cv: real blocking (no busy-wait), and the wait predicate means
  the "spurious wakeup" requirement is satisfied for free.
- `count()` locking: yes, even though it's only a snapshot — it keeps the
  object TSan-clean when threads read the balance concurrently.

## Make it harder (optional — not covered by the tests)

- **Fairness:** the mutex+cv design isn't FIFO — plan waiters get no ordering
  guarantee. Implement a `FairSemaphore` (a second `n_waiters_` counter and
  per-waiter flags) so tokens go to the longest-waiting thread.
- **Timeout:** add `bool wait_for(milliseconds)` that acquires iff a token
  appears within the deadline.
- **Binary check:** `std::binary_semaphore` semantics (max count 1) — reserve
  the budget size and make `release()` a no-op at the cap.
- **Benchmark:** fan-in a thousand producers against `Semaphore(4)` in the
  `tsan;stress` build and report throughput vs. a plain mutex.

## Files

- Stub: `src/semaphore.cpp`
- Tests: `test/test_semaphore.cpp`
- Reference: `SOLUTION.md`