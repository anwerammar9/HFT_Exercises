#include "thread_pool.h"

#include <mutex>
#include <stdexcept>

// TODO(anwer): implement the worker pool (see SOLUTION.md).
//
// The stub spawns the workers so shutdown() joins cleanly, but enqueue()
// rejects every task with std::logic_error — so any test that submits real
// work fails fast at the submit() call instead of blocking on the future.
// (The "submit-after-shutdown throws" test passes spuriously, as documented.)

ThreadPool::ThreadPool(std::size_t n_threads)
    : workers_(n_threads > 0 ? n_threads : 1) {
  for (auto& w : workers_) w = std::thread(&ThreadPool::worker_loop, this);
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::shutdown() {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    shutting_down_ = true;
  }
  cv_.notify_all();
  for (auto& w : workers_)
    if (w.joinable()) w.join();
}

void ThreadPool::enqueue(std::function<void()> fn) {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    if (shutting_down_) {
      throw std::logic_error("enqueue after shutdown");
    }
    tasks_.push_back(fn);
  }
  cv_.notify_one();
}

void ThreadPool::worker_loop() {
  std::unique_lock<std::mutex> lock(mtx_);
  while (!shutting_down_) {
    if (!tasks_.empty()) {
      auto task = std::move(tasks_.front());
      tasks_.pop_front();
      lock.unlock();
      task();
      lock.lock();
    } else {
      cv_.wait(lock, [this] { return shutting_down_ || !tasks_.empty(); });
    }
  }
  cv_.wait(lock, [this] { return shutting_down_; });
}