#include "serial_executor.h"

#include <stdexcept>

// TODO(anwer): implement the executor (see SOLUTION.md).
//
//   SerialExecutor::SerialExecutor() { worker_ = std::thread(&SerialExecutor::worker_loop, this); }
//
//   void SerialExecutor::enqueue(std::function<void()> job) {
//     { std::lock_guard<std::mutex> lock(mtx_);
//       if (shutting_down_) throw std::logic_error("serial executor is shutting down");
//       tasks_.push_back(std::move(job)); }
//     cv_.notify_one();
//   }
//
//   void SerialExecutor::worker_loop() {
//     std::unique_lock<std::mutex> lock(mtx_);
//     for (;;) {
//       if (!tasks_.empty()) {
//         auto job = std::move(tasks_.front()); tasks_.pop_front();
//         ++in_flight_;
//         lock.unlock();
//         job();                 // run OUTSIDE the lock
//         lock.lock();
//         --in_flight_;
//         cv_.notify_all();      // a drain() may be waiting on in_flight_ == 0
//       } else if (shutting_down_) {
//         break;
//       } else {
//         cv_.wait(lock);
//       }
//     }
//   }
//
//   void SerialExecutor::drain() { ... cv_.wait(lock, tasks_.empty() && in_flight_ == 0); }
//   void SerialExecutor::shutdown() { flag on; notify; join worker_; }
//   std::size_t SerialExecutor::pending() const { lock; return tasks_.size() + in_flight_; }

SerialExecutor::SerialExecutor() {}

SerialExecutor::~SerialExecutor() { shutdown(); }

void SerialExecutor::enqueue(std::function<void()> /*job*/) {
  throw std::logic_error("not implemented");
}

void SerialExecutor::worker_loop() {}

void SerialExecutor::drain() {}

void SerialExecutor::shutdown() {}

std::size_t SerialExecutor::pending() const { return 0; }