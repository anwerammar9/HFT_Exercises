# Exercise containers/03_priority_queue (ex20) — Priority Queue (Implicit Heap) (Task)

## The problem (in plain words)

Whenever a system must "serve the highest-priority item first" — matching at
price-time, a latency scheduler, a work queue — it uses a priority queue.
Implement one from scratch as an **implicit binary max-heap** over a plain
`std::vector`: elements live at `parent(i) = (i−1)/2`, `children(i) = 2i+1,
2i+2`, with **sift-up** on insert and **sift-down** on pop. No tree nodes, no
pointers — just array indices.

## Requirements (what the tests check)

1. Semantics match `std::priority_queue`: with `Compare = std::less<T>`,
   `top()` is the **largest** element; `Compare(a, b)` is true when `a` sorts
   strictly higher than `b`.
2. The heap invariant holds after every `push`/`pop` (verified by the tests).
3. `push`/`pop` are **O(log n)**; `top`/`size`/`empty` are **O(1)**.
4. `pop()` removes the top; **pop order is descending by `Compare`**.
5. Equal-priority elements may come out in any order — heaps are not stable and
   the tests never rely on them being.
6. `top()` on an empty queue **throws `std::out_of_range`** — a documented,
   safer deviation from `std::priority_queue`'s UB.
7. `emplace(...)` forwards arguments straight into a `T` constructor; `clear()`
   empties the queue.

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

The stub lives in `include/priority_queue.h` (a template) — implement inline.

## How to think about it (suggested design)

- Store elements in `heap_` (a `std::vector<T>`); the comparator is `cmp_`.
- **push:** append at the end, then *sift up* — while the node is better than
  its parent (`cmp_` says so), swap with the parent.
- **pop:** swap the root with the last element, drop the last, then *sift
  down* — while a child is worse/better, swap with the *best* child so the heap
  invariant is restored. (Pick the child correctly: compare both children first.)
- Everything is `while` loops over indices — the whole point is that it
  compiles to nothing but array arithmetic.

## Make it harder (optional — not covered by the tests)

- **`decrease_key`:** a heap of `(key, priority)` pairs with a separate
  `std::unordered_map` tracking where each key sits, so a priority *update*
  re-heapifies in O(log n) — what real Dijkstra implementations need.
- **d-ary heap:** parameterize `d` (children `d·i+1 … d·i+d`) and benchmark 4-ary
  vs. binary for cache behavior.
- **Heapify-from-range:** a constructor that builds a heap from a range in O(n)
  (build-heap by sifting internal nodes down instead of pushing one by one).
- **`erase_any`:** remove an arbitrary element (via the same "swap with last +
  restore" trick you wrote for pop).
- **Reservation:** make `heap_` `reserve(n)`-aware via a `reserve(size_t)`.

## Files

- Stub: `include/priority_queue.h` (template — implement inline)
- Tests: `test/test_priority_queue.cpp`
- Reference: `SOLUTION.md`