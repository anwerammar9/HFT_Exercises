#ifndef EXERCISE43_HEAP_H_
#define EXERCISE43_HEAP_H_

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

// From-scratch implicit binary heap. This is the *heap* exercise (distinct
// from the PriorityQueue in Exercise 20, which is the thin container on top):
// along with sift-up/sift-down it builds a heap FROM a range in O(n) (the
// real algorithmic trick), removes an arbitrary index in O(log n), and ships a
// from-scratch in-place heapsort.
//
// Semantics (matching std::priority_queue's conventions):
//   - with Compare = std::less<T> it is a MAX-heap: top() is the largest.
//   - Compare(a, b) is true when a sorts strictly LOWER than b (a would be
//     popped after b).
//   - parent(i) = (i-1)/2, children(i) = 2i+1, 2i+2.
//   - top()/pop() on an empty heap throw std::out_of_range (documented, safer
//     deviation from std::priority_queue's UB).
//   - erase_at(i) removes the element at array index i by the swap-with-last
//     trick and restores the invariant in O(log n) (used by d-ary/indexed-heap
//     Dijkstra implementations).
//   - array() exposes the raw storage so tests can assert the strict heap
//     invariant directly (std::is_heap) instead of only pop order.
//
// TODO(anwer): implement sift-up / sift-down / build-from-range / erase_at /
// heapsort (see SOLUTION.md). The stub stores nothing and throws on top/pop,
// so every mutation/order test runs RED without hanging or crashing.

template <typename T, typename Compare = std::less<T>>
class BinaryHeap {
 public:
  using value_type = T;
  using size_type = std::size_t;

  BinaryHeap() = default;
  explicit BinaryHeap(Compare) {}

  template <typename InputIt>
  BinaryHeap(InputIt /*first*/, InputIt /*last*/, Compare comp = Compare())
      : cmp_(std::move(comp)) {}

  size_type size() const noexcept { return 0; }
  bool empty() const noexcept { return true; }

  void push(const T&) {}
  void push(T&&) {}
  template <typename... Args>
  void emplace(Args&&...) {}

  const T& top() const { throw std::out_of_range("BinaryHeap::top on empty"); }
  void pop() {}

  void erase_at(size_type) {}
  void replace(const T&) {}
  void clear() noexcept {}

  const std::vector<T>& array() const noexcept { return heap_; }

 private:
  Compare cmp_{};
  std::vector<T> heap_;
};

// In-place heapsort (ascending). TODO(anwer): build a max-heap then repeatedly
// move the max to the end (see SOLUTION.md). Stub is a no-op.
template <typename T>
void heapsort(std::vector<T>& /*values*/) {}

#endif  // EXERCISE43_HEAP_H_