# Exercise concurrency/04_thread_pool (ex14) — Thread Pool (Task)

## The problem (in plain words)

A fixed set of worker threads that execute submitted tasks. `submit(f, args...)`
must run `f(args...)` **exactly once** on a worker and hand back a real
`std::future` — and `shutdown()` has to **drain every already-queued task to
completion** before the workers are joined: no task may be dropped, and
`shutdown()` must not return until everything submitted before it has finished.

## Requirements (what the tests check)

1. `submit(f, args...)` executes `f(args...)` exactly once and returns a future
   carrying the result.
2. An exception thrown inside a task is captured and re-thrown by
   `future.get()`.
3. A pool with ≥ 2 workers actually runs tasks **concurrently** (the tests use
   an atomic "currently executing" count + a barrier to prove it).
4. `shutdown()` stops accepting new work, **runs every already-queued task to
   completion**, then joins the workers — no dropped tasks, no deadlock (M
   submits all complete before `shutdown()` returns). Idempotent (calling it
   twice is fine).
5. `submit` after `shutdown()` is rejected with a thrown `std::logic_error`.
6. `~ThreadPool()` calls `shutdown()` automatically.

## Public API

```cpp
class ThreadPool {
  explicit ThreadPool(std::size_t n_threads);
  ~ThreadPool();
  template <class F, class... Args>
  auto submit(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>>;  // already written
  void shutdown();
};
```

## How to think about it (suggested design)

- `submit`'s body is **already implemented** — it packs the callable together
  with its captured arguments and wraps it so a `std::future` comes back. Your
  job is the machinery behind it:
  1. a task queue, e.g. `std::deque<std::function<void()>> tasks_`;
  2. a `std::mutex mtx_` + `std::condition_variable cv_` pairing;
  3. a `shutting_down_` flag;
  4. `worker_loop()` (start the `n_threads` threads in the constructor) that
     waits on `cv_`, pops a task, and runs it;
  5. `enqueue(fn)` that inserts under the lock and notifies one waiter.
- `shutdown()`: set the flag under the lock, notify **all** workers, then
  `join` every thread. Because the workers drain the queue *before* exiting,
  queued work is guaranteed to run.
- This is classic mutex+CV plumbing — the "boring, correct" default is the
  right answer here.

## Make it harder (optional — not covered by the tests)

- **Priority tasks:** a bounded priority queue (reuse Exercise 20) so urgent
  tasks overtake queued ones.
- **`try_submit`:** a non-blocking variant that returns `std::nullopt` instead
  of waiting when the queue is capped.
- **Work-stealing:** give each worker its own deque + a global steal target,
  and micro-benchmark load balancing vs. the single-queue version.
- **`wait_all()`:** a method that blocks until every currently-submitted task
  is done (a counting synchronization primitive), and a `task_count()` metric.
- **Move-only tasks:** exercise the pool with `std::unique_ptr` payload tasks to
  prove the packaging hands arguments through correctly.

## Files

- Stub: `src/thread_pool.cpp`
- Tests: `test/test_thread_pool.cpp`
- Reference: `SOLUTION.md`