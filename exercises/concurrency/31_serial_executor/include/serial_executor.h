#ifndef EXERCISE31_SERIAL_EXECUTOR_H_
#define EXERCISE31_SERIAL_EXECUTOR_H_

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

// Serial (single-worker) executor: FIFO task execution in submission order,
// one task at a time, on a private worker thread.
//
// This is the coordination primitive behind every serial async engine: an
// embedded event loop, a single-threaded market-data pipeline, an "asynchronous
// mutex" that orders callbacks from many threads onto one owning thread.
//
// API:
//   SerialExecutor ex;
//   std::future<void> f = ex.submit([]{ ... });                    // void task
//   std::future<int>  g = ex.submit([](int a){ return a * 2; }, 21);  // args+ret
//   ex.drain();      // block until every submitted task has COMPLETED
//   ex.shutdown();   // stop accepting work, run queued tasks, join the worker
//                    // (same as letting the destructor do it, but explicit)
//
// Contract:
//   - Tasks run EXACTLY once, in submission (FIFO) order, one at a time.
//   - Exceptions inside a task are caught by std::packaged_task and re-thrown
//     from future.get(); they never kill the executor or the queue.
//   - submit() is safe from any thread, concurrently. A task may itself submit
//     (reentrancy) — the new task lands at the tail.
//   - drain() is only safe from a NON-worker thread (the worker draining
//     itself would wait forever).
//   - submit() AFTER shutdown() throws std::logic_error.
//   - ~SerialExecutor() drains and joins.
//
// TODO(anwer): implement the class out-of-line in src/serial_executor.cpp
//   (submit() is the completed header template below):
//   - The worker a task order queue: mutex + condition_variable +
//     std::deque<std::function<void()>>. Worker: pop task, track `in_flight_`,
//     run OUTSIDE the lock (a slow task must not stall the queue), decrement.
//   - drain(): cv wait until tasks_ empty AND in_flight_ == 0.
//   - shutdown(): flag on, cv notify_all, join the thread.

class SerialExecutor {
 public:
  SerialExecutor();
  ~SerialExecutor();

  SerialExecutor(const SerialExecutor&) = delete;
  SerialExecutor& operator=(const SerialExecutor&) = delete;

  SerialExecutor(SerialExecutor&&) = delete;
  SerialExecutor& operator=(SerialExecutor&&) = delete;

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using R = std::invoke_result_t<F, Args...>;
    // Packages f(args...) so exceptions are captured into the future, exactly
    // like ThreadPool::submit in 14.
    auto task = std::make_shared<std::packaged_task<R()>>(
        [f = std::forward<F>(f),
         args = std::make_tuple(std::forward<Args>(args)...)]() mutable
            -> R { return std::apply(std::move(f), std::move(args)); });
    std::future<R> result = task->get_future();
    enqueue([task] { (*task)(); });
    return result;
  }

  // Blocks (caller thread) until every submitted task has completed.
  void drain();
  // Stops accepting work, runs everything queued, joins the worker. Idempotent.
  void shutdown();
  // How many tasks are queued or in flight (a snapshot).
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

#endif  // EXERCISE31_SERIAL_EXECUTOR_H_