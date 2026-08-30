# Exercise 20 — Priority Queue (Implicit Heap) (Reference Solution)

**What you implement:** a from-scratch `std::priority_queue` as an implicit
binary max-heap: `parent(i)=(i-1)/2`, `children(i)=2i+1, 2i+2`, sift-up on
insert, sift-down on pop, with `Compare(a,b)` true when `a` sorts strictly
*higher* than `b` (i.e. the top is the largest element under `std::less`).

**Approach**
- `push`: append at the tail, then sift-up — while `cmp_(parent, child)`
  (parent has lower priority) swap upward.
- `pop`: swap top with last, pop_back, then sift-down from the root choosing
  the child with higher priority (`cmp_(left, right)` selects right) and swap
  only while the child outranks the parent.
- `top()`: `heap_.front()`, throwing `std::out_of_range` when empty — a
  documented safer deviation from `std::priority_queue`'s UB.
- `emplace`: forward args into the tail then sift-up. `clear()` reuses the
  underlying vector clear.
- Complexity: O(log n) push/pop, O(1) top/size, max-heap invariant holds for
  every mutation.

## Reference API — `include/priority_queue.h`
#ifndef EXERCISE20_PRIORITY_QUEUE_H_
#define EXERCISE20_PRIORITY_QUEUE_H_

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

// From-scratch std::priority_queue: implicit binary max-heap over contiguous
// storage. Used in the HFT layers wherever "serve the highest-priority item
// first" appears (matching at price-time, latency/message schedulers).
//
// Semantics match std::priority_queue:
//   - with Compare = std::less<T>, top() is the LARGEST element;
//   - Compare(a, b) must return true when a sorts strictly after b
//     (a has lower priority), i.e. the heap is ordered by Compare.
//   - pop() removes the current top in O(log n); the overall pop order is
//     descending by Compare.
//   - equal-priority elements come out in an implementation-defined order
//     (heaps are not stable) — tests never rely on it.
//   - top() on an empty queue throws std::out_of_range (a documented, safer
//     deviation from the UB of std::priority_queue).
//
// Required design decisions (pick & document):
//   - implicit tree via array indices: parent(i) = (i-1)/2,
//     children(i) = 2i+1, 2i+2; sift-up on push, sift-down on pop.
//   - container choice and growth (std::vector-backed is fine); all methods
//     must keep the strict heap invariant.

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
  void sift_up(size_type i);
  void sift_down(size_type i);

  Compare cmp_;
  std::vector<T> heap_;
};

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::sift_up(size_type i) {
  while (i > 0) {
    size_type parent = (i - 1) / 2;
    if (!cmp_(heap_[parent], heap_[i])) break;  // parent has >= priority
    std::swap(heap_[parent], heap_[i]);
    i = parent;
  }
}

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::sift_down(size_type i) {
  const size_type n = heap_.size();
  for (;;) {
    size_type child = 2 * i + 1;
    if (child >= n) break;
    if (child + 1 < n && cmp_(heap_[child], heap_[child + 1])) {
      ++child;  // right child has strictly higher priority
    }
    if (!cmp_(heap_[i], heap_[child])) break;
    std::swap(heap_[i], heap_[child]);
    i = child;
  }
}

// ---------------------------------------------------------------------------
// Implementation: implicit max-heap with sift-up / sift-down.
// parent(i) = (i-1)/2; children(i) = 2i+1, 2i+2.
// Invariant: for every parent p and child c, !cmp_(p, c) (a parent is never
// of lower priority than its child), i.e. top() carries the highest priority.
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::push(const T& value) {
  heap_.push_back(value);
  sift_up(heap_.size() - 1);
}

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::push(T&& value) {
  heap_.push_back(std::move(value));
  sift_up(heap_.size() - 1);
}

template <typename T, typename Compare>
template <typename... Args>
void PriorityQueue<T, Compare>::emplace(Args&&... args) {
  heap_.emplace_back(std::forward<Args>(args)...);
  sift_up(heap_.size() - 1);
}

template <typename T, typename Compare>
const T& PriorityQueue<T, Compare>::top() const {
  if (heap_.empty()) throw std::out_of_range("PriorityQueue::top on empty");
  return heap_.front();
}

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::pop() {
  if (heap_.empty()) return;
  std::swap(heap_.front(), heap_.back());
  heap_.pop_back();
  if (!heap_.empty()) sift_down(0);
}

#endif  // EXERCISE20_PRIORITY_QUEUE_H_
## Reference TU — `src/priority_queue.cpp` (explicit instantiation)
#include "priority_queue.h"

#include <string>

// Explicit instantiations: header (stubs included) must really compile for
// both a trivially-copyable type (int) with the default Compare and for a
// heap-string type with a custom comparator.
template class PriorityQueue<int>;
template class PriorityQueue<std::string, std::greater<std::string>>;