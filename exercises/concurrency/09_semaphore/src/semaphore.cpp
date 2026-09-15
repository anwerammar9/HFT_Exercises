#include "counting_semaphore.h"

// TODO(anwer): implement a counting semaphore (see SOLUTION.md).
//
//   - acquire():   std::unique_lock lock(mu_); cv_.wait(lock, [&]{ return n_ > 0; });
//                  --n_;
//   - try_acquire(): std::lock_guard lock(mu_); if (n_ == 0) return false;
//                  --n_; return true;
//   - release():   std::lock_guard lock(mu_); ++n_; cv_.notify_one();
//   - count():     std::lock_guard lock(mu_); return n_;

Semaphore::Semaphore(std::size_t initial) : n_(initial) {}

void Semaphore::acquire()
{

}

bool Semaphore::try_acquire()
{
    return false;
}

void Semaphore::release()
{

}

std::size_t Semaphore::count() const
{
    return 0;
}