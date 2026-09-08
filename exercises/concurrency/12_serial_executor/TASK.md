# Exercise concurrency/12_serial_executor (ex31) — Serial Executor (Task)

## The problem (in plain words)

A **serial executor** is "one queue + one worker thread": every task you throw
at it via `submit()` runs exactly once, **in submission order**, one at a time,
on that private thread, and `submit()` hands you back a `std::future` with the
result. It's the single-threading primitive behind event loops, serial
market-data pipelines, and the "asynchronous mutex" that forces callbacks from
many threads to touch shared state one at a time.

## Requirements (what the tests check)

1. `submit(f, args...)` runs `f(args...)` exactly once, in FIFO submission
   order, and returns a `std::future<...>` holding the result.
2. **Serial property (the discriminator):** with many tasks submitted in a
   burst, the number of tasks executing *concurrently* never exceeds 1 — it
   really is one worker thread.
3. Exceptions thrown inside a task are captured by `std::packaged_task` and
   re-thrown from `future.get()`; the worker survives and keeps executing.
4. `drain()` blocks the caller until every submitted task (including the one
   currently in flight) has **completed**.
5. `shutdown()` stops accepting work, runs everything queued, joins the worker,
   and is idempotent. `~SerialExecutor()` does the same.
6. `submit()` after `shutdown()` throws `std::logic_error`.
7. A task may itself call `submit()` (reentrancy) — the new task lands at the
   tail and still runs.
8. `pending()` reports a snapshot of queued + in-flight work (>= 2 right after
   submitting 3 long-ish tasks is a test).

## Public API

```cpp
class SerialExecutor {
 public:
  SerialExecutor();
  ~SerialExecutor();

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>>;  // header; complete

  void drain();       // block until all submitted work has completed
  void shutdown();    // stop, drain, join; idempotent
  std::size_t pending() const;
 private:
  void enqueue(std::function<void()> job);
  void worker_loop();
  std::thread worker_;
  std::deque<std::function<void()>> tasks_;
  mutable std::mutex mtx_;
  std::condition_variable cv_;
  bool shutting_down_ = false;
  std::size_t in_flight_ = 0;
};
```

## How to think about it (suggested design)

- `submit()` is **already written** in the header (mirrors `ThreadPool::submit`
  in Exercise 14): it wraps your callable in a `std::packaged_task<R()>` —
  the wrapper that captures exceptions into the future — and calls the private
  `enqueue()`. Your job is the skeleton beneath it.
- The worker thread structs the whole exercise:
  - `worker_loop()`: `unique_lock(mtx_)`; if `tasks_` has a job, pop it,
    `++in_flight_`, **unlock**, run the job, re-lock, `--in_flight_` and
    `notify_all()`; if `shutting_down_` and `tasks_` empty → break; else
    `cv_.wait(lock)`.
  - Running the task *outside* the lock is the subtle bit: a slow task must not
    stall `enqueue()` or the drain check.
- `enqueue()`: throw `logic_error` when `shutting_down_` (rejects late work),
  `push_back`, `notify_one`.
- `drain()`: `cv_.wait(lock, tasks_.empty() && in_flight_ == 0)` — two
  conditions because a task is "not done" both while queued **and** while
  running.
- `shutdown()`: set the flag, `notify_all()`, `join()`; guard with a
  `shutting_down_` re-check so a double call doesn't double-join.
- The destructor calls `shutdown()` — that's how the "destructor ran all 100
  tasks" test passes.

## Make it harder (optional — not covered by the tests)

- **Priority inbox:** give tasks a priority (a `std::multimap` or bucketed
  queues instead of a deque) so a market-close flush beats a stats task — but
  still exactly-one + serial.
- **Timer/clock tasks:** a `submit_at(deadline, f, args...)` and
  `submit_after(duration, ...)` that timestamps reorder into a min-heap (iken
  Exercise 10's wheel for the wakeup).
- **Cancel pending:** a token-returning `submit` whose `cancel()` removes an
  unstarted task and makes its `future.get()` immediately throw
  `std::future_error` with `future_errc::broken_promise`.
- **Fairness counter:** track tasks-per-submitter and reject one sender's
  floods — the real fix for a noisy neighbor on a shared event loop.
- **Benchmark:** throughput of the submit→drain cycle at 1/2/4/8 producer
  threads under TSan; compare `notify_one` vs `notify_all` on the worker
  side.

## Files

- Stub: `src/serial_executor.cpp`
- Tests: `test/test_serial_executor.cpp`
- Reference: `SOLUTION.md`