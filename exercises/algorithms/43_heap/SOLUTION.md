# Exercise 43 — Binary Heap (Build-Heap / Erase-At / Heapsort) (Reference Solution)

**What you implement:** a from-scratch implicit binary heap that goes beyond the
push/pop container of Exercise 20: an O(n) **build-from-range** heapify, O(1)
`top`, O(log n) `push`/`pop`/`replace`, an **`erase_at(index)`** that removes an
arbitrary element in O(log n) (the swap-with-last trick indexed heaps need for
Dijkstra/priority updates), and a hand-rolled in-place ascending **heapsort**.

**Approach**
- **Conventions:** `parent(i) = (i-1)/2`, `children(i) = 2i+1, 2i+2`; `Compare`
  matches `std::priority_queue` — `cmp_(a,b)` is true when `a` sorts *lower*
  (popped after `b`), so with `std::less` the heap is a max-heap. (The original
  authoring mistake was to invert this and silently build a min-heap.)
- **push:** append, then **sift-up** while the parent sorts lower than the
  child. **pop:** swap root↔last, pop last, **sift-down**, always choosing the
  child the comparator ranks higher.
- **Build-from-range:** `BinaryHeap(first, last)` copies then heapifies O(n) by
  sift-downing only the internal nodes `i = n/2-1 … 0` (not n×push → O(n log n)).
- **erase_at(i):** swap `heap_[i]`↔last, pop, then sift-up *and* sift-down at
  `i` — one of them is a no-op, so an arbitrary index is restored in O(log n).
  Out-of-range throws `std::out_of_range`.
- **replace(v):** pop-then-push in one round-trip: overwrite the root and
  sift-down (a schedulers' re-arm primitive).
- **heapsort(values):** from scratch — build the in-heap max-heap, then swap the
  max to the back and sift-down the shrinking prefix, yielding ascending order.
  `array()` is exposed so tests assert `std::is_heap` on the raw storage.

## Reference API — `include/heap.h`
#ifndef EXERCISE43_HEAP_H_
#define EXERCISE43_HEAP_H_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
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
  explicit BinaryHeap(Compare comp) : cmp_(std::move(comp)) {}

  // Build the heap from a range in O(n) (heapify internal nodes, not n×push).
  template <typename InputIt>
  BinaryHeap(InputIt first, InputIt last,
             Compare comp = Compare())
      : cmp_(std::move(comp)), heap_(first, last) {
    build_heap();
  }

  size_type size() const noexcept { return heap_.size(); }
  bool empty() const noexcept { return heap_.empty(); }

  void push(const T& value) { heap_.push_back(value); sift_up(heap_.size() - 1); }
  void push(T&& value) { heap_.push_back(std::move(value)); sift_up(heap_.size() - 1); }
  template <typename... Args>
  void emplace(Args&&... args) {
    heap_.emplace_back(std::forward<Args>(args)...);
    sift_up(heap_.size() - 1);
  }

  const T& top() const {
    if (heap_.empty()) throw std::out_of_range("BinaryHeap::top on empty");
    return heap_.front();
  }

  void pop() {
    if (heap_.empty()) throw std::out_of_range("BinaryHeap::pop on empty");
    std::swap(heap_.front(), heap_.back());
    heap_.pop_back();
    if (!heap_.empty()) sift_down(0);
  }

  // Remove heap_[i] in O(log n): swap with the last element, drop it, then
  // restore — sift up OR down depending on how the swap disturbed the heap.
  void erase_at(size_type i) {
    if (i >= heap_.size()) throw std::out_of_range("BinaryHeap::erase_at OOB");
    std::swap(heap_[i], heap_.back());
    heap_.pop_back();
    if (i >= heap_.size()) return;  // erased the last element
    sift_up(i);
    sift_down(i);
  }

  // pop + push in one O(log n) round-trip (what a scheduler does when it
  // re-arms the next deadline).
  void replace(const T& value) {
    if (heap_.empty()) {
      push(value);
      return;
    }
    heap_.front() = value;
    sift_down(0);
  }

  void clear() noexcept { heap_.clear(); }

  // Exposed for tests: the raw heap array (heap-ordered per cmp_, i.e.
  // std::is_heap with the matching comparator must hold).
  const std::vector<T>& array() const noexcept { return heap_; }

 private:
  // build_heap / sift_* follow the std::priority_queue convention: phi(a,b)
  // is true when `a` must be popped AFTER `b` (a sorts LOWER), so with
  // Compare = std::less<T> the top is the LARGEST element. A node climbs
  // while its parent sorts lower than it and sinks while a child sorts
  // higher than it.
  void build_heap() {
    if (heap_.size() < 2) return;
    for (size_type i = heap_.size() / 2; i-- > 0;) sift_down(i);
  }

  void sift_up(size_type i) {
    while (i > 0) {
      size_type p = (i - 1) / 2;
      if (!cmp_(heap_[p], heap_[i])) break;  // parent no longer sorts lower
      std::swap(heap_[i], heap_[p]);
      i = p;
    }
  }

  void sift_down(size_type i) {
    const size_type n = heap_.size();
    for (;;) {
      size_type best = i;
      size_type l = 2 * i + 1;
      size_type r = 2 * i + 2;
      if (l < n && cmp_(heap_[best], heap_[l])) best = l;
      if (r < n && cmp_(heap_[best], heap_[r])) best = r;
      if (best == i) return;
      std::swap(heap_[i], heap_[best]);
      i = best;
    }
  }

  // Compare(a, b) is true when a sorts strictly LOWER than b (a is popped
  // after b): with Compare = std::less<T> the top is the largest element.
  Compare cmp_;
  std::vector<T> heap_;
};

// In-place heapsort (ascending). Builds a max-heap over values[0,n), then
// repeatedly moves the current max to the end and sift-downs the shrinking
// prefix. Computed entirely by hand — no std::heap_* calls.
template <typename T>
void heapsort(std::vector<T>& values) {
  const std::size_t n = values.size();
  if (n < 2) return;
  auto less = std::less<T>{};
  auto sift_down = [&](std::size_t i, std::size_t stop) {
    for (;;) {
      std::size_t best = i;
      std::size_t l = 2 * i + 1;
      std::size_t r = 2 * i + 2;
      if (l < stop && less(values[i], values[l])) best = l;
      if (r < stop && less(values[best], values[r])) best = r;
      if (best == i) return;
      std::swap(values[i], values[best]);
      i = best;
    }
  };
  for (std::size_t i = n / 2; i-- > 0;) sift_down(i, n);   // build max-heap
  for (std::size_t end = n - 1; end > 0; --end) {
    std::swap(values[0], values[end]);                      // max → its final slot
    sift_down(0, end);                                       // re-heapify prefix
  }
}

#endif  // EXERCISE43_HEAP_H_