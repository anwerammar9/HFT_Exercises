# Exercise 38 — Linked List (Doubly-Linked, Intrusive) (Task)

## The problem (in plain words)

A limit order book bookkeeps **resting orders per price level as a queue**:
new orders append at the tail, the first-in order gets filled, and a *cancel*
or *fill* can strike **anywhere in the middle**. The classic way to get
O(1) push at the tail, O(1) pop at the head AND O(1) removal from the middle
(the cost model of a resting-order queue) is a **doubly-linked list with
sentinels**. Implement one from scratch — the *sentinels* are plain links owned
by the list, payload nodes hold a `T`, and every operation is pointer rewiring.

## Requirements (what the tests check)

1. `push_back`, `push_front`, `pop_back`, `pop_front`, `size`, `empty`,
   `clear` — with `size()`/`empty()` O(1) (the size must *mirror* the chain).
2. `erase(iterator)` removes **one node in O(1) without searching** and returns
   the iterator to the **next** node, so `it = l.erase(it);` is a valid loop.
   Erasing `end()` is a harmless no-op; erasing the last node yields `end()`.
3. `front()`/`back()` return references and **throw `std::out_of_range`** on an
   empty list (a documented, safer deviation from `std::list`'s UB).
4. `reverse()` rewires in place — after it, iteration order is reversed and the
   list is still fully valid (all O(n)).
5. Iterators are **bidirectional**: `++`/`--` both directions, `--end()`
   is the last element, `begin() == end()` iff empty, range-for works (const
   and non-const).
6. Writing through an iterator updates the list (iterator `operator*` yields a
   real element reference).
7. Value semantics: the copy constructor is **deep** (mutating a copy never
   touches the original); move leaves the source empty; `swap` is O(1); the
   destructor frees every node (no leaks).

## Public API

```cpp
template <typename T>
class LinkedList {
  LinkedList();  LinkedList(const LinkedList&);  LinkedList(LinkedList&&);
  LinkedList& operator=(const LinkedList&);  LinkedList& operator=(LinkedList&&);
  ~LinkedList();  void swap(LinkedList&) noexcept;
  std::size_t size() const noexcept;  bool empty() const noexcept;
  T& front();  const T& front() const;  T& back();  const T& back() const;
  void push_back(const T&);  void push_back(T&&);
  void push_front(const T&);  void push_front(T&&);
  void pop_front();  void pop_back();  void clear() noexcept;
  iterator erase(iterator);  void reverse();
  iterator begin();  iterator end();
  const_iterator begin() const;  const_iterator end() const;
};
```

The stub lives in `include/linked_list.h` (a template) — implement inline.

## How to think about it (suggested design)

- `head_`/`tail_` are plain `Link` members; `begin()` is `head_.next`, `end()`
  is `&tail_`, so an empty list costs zero allocations.
- A payload node is `struct Node : Link` with a `T`; dereferencing an iterator
  is a `static_cast<Node*>`.
- **erase(it):** `it.prev->next = it.next; it.next->prev = it.prev;` splice out,
  `delete`, return the successor — the whole point of the exercise.
- **Move/swap:** move the *payload region* into your own sentinels — never
  cross-link two containers' sentinels (they live inside the objects).
- **reverse():** swap `prev`/`next` per node, then re-attach the sentinels.

## Make it harder (optional — not covered by the tests)

- **O(1) tail tracking for `reverse`:** keep a count/caching of the tail after a
  `reverse` (avoid re-walking).
- **Splice:** move a whole `[first, last)` range from one list to another in
  O(1) — mirror `std::list::splice`.
- **Arena allocation:** allocate nodes out of a growable block arena instead of
  `new` per node and benchmark (this is what a real matching engine does).

## Files

- Stub: `include/linked_list.h` (template — implement inline)
- Tests: `test/test_linked_list.cpp`
- Reference: `SOLUTION.md`
