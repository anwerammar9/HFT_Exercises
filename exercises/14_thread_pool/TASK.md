# Exercise 14 — Thread Pool (Task)

## Problem
A fixed-size worker pool: `submit` wraps a task so it runs exactly once on a
worker and hands back a real `std::future<R>`. `shutdown()` drains **all**
queued work before the workers exit.

## Requirements (what the tests check)
1. `submit(f, args...)` executes `f(args...)` exactly once and returns a
   `std::future` with the result.
2. Exceptions thrown inside a task propagate through `future.get()`.
3. A pool with ≥ 2 workers actually runs tasks concurrently (a
   barrier/"currently executing" atomic count proves it).
4. `shutdown()` stops accepting work, **runs every already-queued task to
   completion**, then joins the workers. No tasks dropped, no deadlock
   (M submits all run before shutdown returns). Idempotent.
5. `submit` after shutdown is rejected: throws `std::logic_error`.
6. `~ThreadPool()` calls `shutdown()`.

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

## Design notes
`submit`'s body is already implemented (packaged_task + std::apply). You must
add: the `worker_loop()`/`enqueue()` on the provided
`tasks_`/`mtx_`/`cv_`/`shutting_down_` members (mutex + condition_variable +
deque is the simple, correct default) and start the worker threads in the ctor.

## Files
- Stub: `src/thread_pool.cpp`
- Tests: `test/test_thread_pool.cpp`
- Reference: `SOLUTION.md`