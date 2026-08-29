# Exercise 43 — Binary Heap (Build-Heap / Erase-At / Heapsort) (Task)

## The problem (in plain words)

Exercise 20 gave you the **priority queue container** on top of an implicit
heap. This exercise is the heap itself — the parts a real hot-path queue needs
but a push/pop container hides: **build a heap from a range in O(n)** (not n
push → O(n log n)), remove an **arbitrary index in O(log n)** (the
swap-with-last trick every indexed heap / Dijkstra needs), `replace` the root
in one round-trip, and sort in place with a from-scratch **heapsort**.

## Requirements (what the tests check)

1. Conventions match `std::priority_queue`: with `Compare = std::less<T>` it is
   a **max-heap**; `Compare(a, b)` is true when `a` sorts *lower* than `b`
   (popped after it).
2. Default-constructible; `push`/`emplace`; `top()` O(1) (throws
   `std::out_of_range` when empty); `pop()` restores the invariant; `clear()`.
3. **Build-from-range** `BinaryHeap(first, last)` heapifies in O(n).
4. `erase_at(i)` removes element at array index `i` in O(log n) — bad index
   throws `std::out_of_range`.
5. `replace(v)`: remove the max and insert `v` in one O(log n) round-trip.
6. `array()` exposes the raw storage so tests can assert the **strict** heap
   invariant (`std::is_heap`) after every mutation.
7. `heapsort(values)`: ascending sort, from scratch, in place, **O(n log n)**
   in all cases.

## Public API

```cpp
template <typename T, typename Compare = std::less<T>>
class BinaryHeap {
  BinaryHeap();  explicit BinaryHeap(Compare comp);
  template <typename InputIt> BinaryHeap(InputIt first, InputIt last, Compare comp = Compare());
  size_type size() const noexcept;  bool empty() const noexcept;
  void push(const T&);  void push(T&&);
  template <typename... Args> void emplace(Args&&...);
  const T& top() const;  void pop();
  void erase_at(size_type i);  void replace(const T& v);  void clear() noexcept;
  const std::vector<T>& array() const noexcept;
};

template <typename T> void heapsort(std::vector<T>& values);
```

The stub lives in `include/heap.h` (templates) — implement inline.

## How to think about it (suggested design)

- `parent(i) = (i-1)/2`, `children(i) = 2i+1, 2i+2`; store in `heap_`.
- **sift-up** (push): while the parent sorts lower than the child, swap.
- **sift-down** (pop): swap root with last, drop last, then repeatedly swap with
  the *best* child.
- **Build-from-range:** copy, then sift-down only the internal nodes
  `i = n/2-1 … 0` — that is what makes it O(n).
- **erase_at(i):** swap `heap_[i]` with the last, pop, then sift-up *and*
  sift-down at `i` — one of them is a no-op, so the index is restored in
  O(log n).
- **heapsort:** build a max-heap (== the `BinaryHeap` build), then repeatedly
  swap the max to the back and sift-down the *shrinking* prefix → ascending.

## Make it harder (optional — not covered by the tests)

- **Indexed heap:** track `std::unordered_map<T, size_t>` positions and add
  `decrease_key`/`increase_key` — the structure behind Dijkstra's decrease-key.
- **d-ary heap:** parameterize the arity `d` and benchmark 4-ary vs. binary
  (children `d·i+1 … d·i+d`).
- **Heap at fixed memory:** build the heap over an externally owned buffer with
  no allocation (`std::span`).
- **Partial sort:** `heapsort` only the first `k` elements and stop once those
  `k` are in place — O(n + k log n) "top-k most volatile symbols".

## Files

- Stub: `include/heap.h` (templates — implement inline)
- Tests: `test/test_heap.cpp`
- Reference: `SOLUTION.md`
