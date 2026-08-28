# Exercise 20 — Priority Queue (Implicit Heap) (Task)

## Problem
A from-scratch `std::priority_queue`, implemented as an **implicit binary
max-heap** over contiguous storage: `parent(i) = (i−1)/2`,
`children(i) = 2i+1, 2i+2`; sift-up on insert, sift-down on pop.

## Requirements (what the tests check)
1. Semantics match `std::priority_queue`: with `Compare = std::less<T>`,
   `top()` is the LARGEST element; `Compare(a,b)` is true when `a` sorts
   strictly higher than `b`.
2. Heap invariant holds after every `push`/`pop`.
3. `push`/`pop` are O(log n); `top`/`size`/`empty` are O(1).
4. `pop()` removes the top; the **pop order is descending by `Compare`**.
5. Equal-priority elements may come out in any order (heaps aren't stable) —
   tests never rely on it.
6. `top()` on an empty queue **throws `std::out_of_range`** (a documented,
   safer deviation from `std::priority_queue`'s UB).
7. `emplace(...)` forwards arguments; `clear()` empties.

## Public API
```cpp
template <typename T, typename Compare = std::less<T>>
class PriorityQueue {
  PriorityQueue();  explicit PriorityQueue(Compare comp);
  size_type size() const;  bool empty() const;
  void push(const T& value);  void push(T&& value);
  template <typename... Args> void emplace(Args&&... args);
  const T& top() const;
  void pop();
  void clear() noexcept;
};
```

## Files
- Stub: `include/priority_queue.h` (template — implement inline)
- Tests: `test/test_priority_queue.cpp`
- Reference: `SOLUTION.md`