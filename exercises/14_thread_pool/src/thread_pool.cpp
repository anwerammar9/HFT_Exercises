#include "thread_pool.h"

#include <functional>
#include <stdexcept>
#include <utility>

// TODO(anwer): implement the pool (see SOLUTION.md).
//
// Queue choice: mutex + condition_variable + std::deque<std::function<void()>>.
// Simple, blocking, correct; contention only at the submit/worker handoff.
//
// Stub: enqueue() rejects every task, so submit() throws std::logic_error
// immediately (RED); workers simply wait for the shutdown flag and exit, so
// shutdown()/~ThreadPool() join cleanly with no queued tasks left behind.

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
  cv_.notify_all();
  for (auto& w : workers_) w.join();
}

void ThreadPool::enqueue(std::function<void()> /*fn*/) {
  throw std::logic_error("ThreadPool::submit: not implemented");  // TODO(anwer)
}

void ThreadPool::worker_loop() {
  std::unique_lock<std::mutex> lock(mtx_);
  cv_.wait(lock, [this] { return shutting_down_; });
}