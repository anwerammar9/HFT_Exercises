# Exercise concurrency/04_thread_pool (ex14) — Thread Pool (Reference Solution)

**What you implement:** a fixed-worker pool with a blocking task queue;
`submit` wraps the callable in a `std::packaged_task` and hands back a real
`std::future<R>`. Requires careful shutdown that drains all queued work.

**Approach**
- Queue: `std::mutex` + `std::condition_variable` + `std::deque<std::function
  <void()>>` — simple, blocking, correct; contention only occurs at the
  boundary (submit/worker handoff), which is fine for this exercise.
- `submit(f, args...)`: capture `f` and `args` in a tuple, build a `std::
  packaged_task<R()>`, get its future, and `enqueue([task]{ (*task)(); })`.
  Worker exceptions are captured by `packaged_task` and rethrown from
  `future.get()`.
- `enqueue`: lock, throw `std::logic_error` if `shutting_down_` (submits after
  shutdown are rejected per contract), else push_back, `notify_one`.
- `worker_loop`: `cv_.wait(pred shutting_down_ || !tasks_.empty())`; pop+run a
  task, or exit when shutting down with an empty queue. `shutdown()` sets the
  flag under the lock, `notify_all`, and joins every worker (idempotent);
  `~ThreadPool()` calls `shutdown()`.
- `n_threads = max(1, n)` guarantees at least one worker.

## Reference API — `include/thread_pool.h`
#ifndef EXERCISE14_THREAD_POOL_H_
#define EXERCISE14_THREAD_POOL_H_

#include <cstddef>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Fixed-size worker pool.
//
// Contract:
//   - `submit(f, args...)` runs `f(args...)` exactly once on a worker and
//     returns a std::future with the result. Exceptions thrown inside the
//     task are captured by std::packaged_task and re-thrown from future.get().
//   - `shutdown()`: stops accepting work, runs every task already queued to
//     completion, then joins the workers. Idempotent.
//   - Submitting AFTER shutdown() (or during) is REJECTED: submit() throws
//     std::logic_error.
//   - ~ThreadPool() calls shutdown().
//
// TODO(anwer): implement in src/thread_pool.cpp.
//   - Worker threads + a task queue. Queue design choices and their tradeoffs
//     (comment your pick):
//       * mutex + condition_variable + std::deque<std::function<void()>>:
//         simple, blocking, correct. Good default.
//       * reuse exercises/21_ring_buffer_mpmc (Vyukov MPMC) + a
//         condition_variable: no lock on the enqueue path, but you lose the
//         queue's bounds (it is bounded, so pick a large size or block).
//   - Member to add: `std::vector<std::thread> workers_` and a guard/flag for
//     the shutdown state.

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t n_threads);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using R = std::invoke_result_t<F, Args...>;
    // std::bind would work, but libstdc++ still emits result_of deprecation
    // warnings through it; a C++17 lambda + std::apply is equally clean.
    auto task = std::make_shared<std::packaged_task<R()>>(
        [f = std::forward<F>(f),
         args = std::make_tuple(std::forward<Args>(args)...)]() mutable
            -> R { return std::apply(std::move(f), std::move(args)); });
    std::future<R> result = task->get_future();
    enqueue([task] { (*task)(); });
    return result;
  }

  void shutdown();

 private:
  void enqueue(std::function<void()> fn);  // internal queue write
  void worker_loop();

  std::vector<std::thread> workers_;
  std::deque<std::function<void()>> tasks_;
  std::mutex mtx_;
  std::condition_variable cv_;
  bool shutting_down_{false};
};

#endif  // EXERCISE14_THREAD_POOL_H_
## Reference implementation — `src/thread_pool.cpp`
#include "thread_pool.h"

#include <functional>
#include <stdexcept>
#include <utility>

// Queue choice: mutex + condition_variable + std::deque<std::function<void()>>.
// Simple, blocking, and correct; contention is acceptable for this exercise and
// the submit path is not the point of the exercise.

ThreadPool::ThreadPool(std::size_t n_threads)
    : workers_(n_threads > 0 ? n_threads : 1) {
  for (auto& w : workers_) w = std::thread(&ThreadPool::worker_loop, this);
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::shutdown() {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    if (shutting_down_) return;  // idempotent
    shutting_down_ = true;
  }
  // Drain every queued task to completion.
  cv_.notify_all();
  for (auto& w : workers_) w.join();
}

void ThreadPool::enqueue(std::function<void()> fn) {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    if (shutting_down_) throw std::logic_error(
        "ThreadPool::submit: pool is shut down");
    tasks_.push_back(std::move(fn));
  }
  cv_.notify_one();
}

void ThreadPool::worker_loop() {
  for (;;) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mtx_);
      cv_.wait(lock, [this] { return shutting_down_ || !tasks_.empty(); });
      if (shutting_down_ && tasks_.empty()) return;
      if (tasks_.empty()) continue;
      task = std::move(tasks_.front());
      tasks_.pop_front();
    }
    task();
  }
}
