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
  
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::shutdown() {
}

void ThreadPool::enqueue(std::function<void()> fn) {
}

void ThreadPool::worker_loop() {

}