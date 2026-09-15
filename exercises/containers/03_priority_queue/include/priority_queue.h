#ifndef EXERCISE20_PRIORITY_QUEUE_H_
#define EXERCISE20_PRIORITY_QUEUE_H_

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

// From-scratch std::priority_queue: implicit binary max-heap over contiguous
// storage. Used in the HFT layers wherever "serve the highest-priority item
// first" appears (matching at price-time, latency/message schedulers).
//
// Semantics match std::priority_queue:
//   - with Compare = std::less<T>, top() is the LARGEST element;
//   - Compare(a, b) must return true when a sorts strictly higher than b.
//   - pop() removes the top in O(log n); pop order is descending by Compare.
//   - equal-priority elements come out in an implementation-defined order
//     (heaps are not stable) — tests never rely on it.
//   - top() on an empty queue throws std::out_of_range (documented, safer
//     deviation from the UB of std::priority_queue).
//
// TODO(anwer): implement with sift-up / sift-down (see SOLUTION.md).
// parent(i) = (i-1)/2, children(i) = 2i+1, 2i+2.

template <typename T, typename Compare = std::less<T>>
class PriorityQueue {
 public:
  using value_type = T;
  using size_type = std::size_t;

  PriorityQueue() = default;
  explicit PriorityQueue(Compare comp) : cmp_(std::move(comp)) {}

  size_type size() const noexcept { return heap_.size(); }
  bool empty() const noexcept { return heap_.empty(); }

  // CORE (TODO(anwer): implement below — sift-up / sift-down).
  void push(const T& value);
  void push(T&& value);
  template <typename... Args>
  void emplace(Args&&... args);
  const T& top() const;
  void pop();
  void clear() noexcept { heap_.clear(); }

 private:
  Compare cmp_;
  std::vector<T> heap_;
};

// ---------------------------------------------------------------------------
// Stub implementation (TODO: replace with the real thing from SOLUTION.md)
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::push(const T& /*value*/) {}

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::push(T&& /*value*/) {}

template <typename T, typename Compare>
template <typename... Args>
void PriorityQueue<T, Compare>::emplace(Args&&... /*args*/) {}

template <typename T, typename Compare>
const T& PriorityQueue<T, Compare>::top() const {
  if (heap_.empty()) throw std::out_of_range("PriorityQueue::top on empty");
  return heap_.front();
}

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::pop() {
  if (heap_.empty()) return;
  heap_.pop_back();
}

#endif  // EXERCISE20_PRIORITY_QUEUE_H_