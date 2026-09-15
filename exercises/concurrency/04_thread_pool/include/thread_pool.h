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
    throw std::logic_error("not implemented");
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