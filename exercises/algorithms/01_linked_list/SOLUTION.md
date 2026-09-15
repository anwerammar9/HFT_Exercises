# Exercise algorithms/01_linked_list (ex38) — Linked List (Doubly-Linked, Intrusive) (Reference Solution)

**What you implement:** a from-scratch doubly-linked list over an *intrusive*
link: the `head_`/`tail_` sentinels are plain `Link` nodes owned by the
container and payload nodes are `Node : Link` holding a `T`. The whole point is
pointer rewiring — O(1) push/pop at both ends, O(1) `erase(iterator)` with no
search (the cost model of a resting-order queue where cancels land mid-chain),
in-place `reverse()`, and full value semantics.

**Approach**
- **Sentinels:** `begin()` is `head_.next`, `end()` is `&tail_`, so an empty
  list satisfies `begin() == end()` with zero allocation and no null checks in
  the hot path. Dereferencing an iterator is a `static_cast<Node*>` since every
  reachable `Link` (except the sentinels) is really a `Node`.
- **Insert:** `push_back`/`push_front` allocate a Node and splice it in;
  `link_back` increments `size_` (a missing `++size_` here was the classic
  first-bug — size must mirror the chain).
- **erase(iterator):** splice out (`n->prev->next = n->next`, `nx->prev =
  n->prev`), `delete`, return the successor so the erase-loop idiom
  `it = l.erase(it)` works; guarding head/tail makes erasing `end()` a no-op.
- **reverse():** swap `prev`/`next` on every payload node, then re-attach the
  sentinels to the old last (new first) and old first (new last). O(n), no
  allocation.
- **Move/copy:** copy is deep (iterate `other` and `push_back`); move and swap
  use a private `steal()` that moves the *payload region* into the container's
  OWN sentinels. This is the piece that is easy to get wrong — a naively
  swapped sentinel `next/prev` pair cross-links between two objects whose
  sentinels live inside them and corrupts empty lists.
- **Contract edge:** `front()`/`back()` throw `std::out_of_range` when empty (a
  documented, safer deviation from `std::list`'s UB).

## Reference API — `include/linked_list.h`
#ifndef EXERCISE38_LINKED_LIST_H_
#define EXERCISE38_LINKED_LIST_H_

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>

// From-scratch doubly-linked list over an intrusive link (head/tail sentinels
// are plain Link nodes; payload nodes derive Link). The whole point is pointer
// rewiring: push/pop at BOTH ends O(1), erase(iterator) O(1) with no search —
// exactly the cost model an order queue at a price level needs (resting orders
// are added at the tail, cancelled from the middle in O(1)).
//
// Contract:
//   - size()/empty() O(1); front()/back() return references and throw
//     std::out_of_range on an empty list (safer than the std::list UB).
//   - push_back/push_front (copy + move), pop_back/pop_front, clear().
//   - erase(iterator) removes one node in O(1) WITHOUT searching and returns
//     the iterator to the next node (so `it = l.erase(it);` is valid).
//   - reverse() rewires in place in O(n).
//   - iterators are bidirectional (++ / -- work, --end() == last);
//     begin()/end() support range-for, const and non-const.
//   - copy is DEEP (mutating the copy never touches the original); move leaves
//     the source empty; the destructor frees every node.
//
// TODO(anwer): implement the real list (see SOLUTION.md). The stub is an
// always-empty sentinel list whose mutators are no-ops, so every mutation/reverse/
// erase/copy test runs RED without allocating or crashing.

template <typename T>
class LinkedList {
  // Intrusive link: sentinels use it, payload nodes extend it with the value.
  struct Link {
    Link* prev;
    Link* next;
  };
  struct Node : Link {
    T value;
    Node(Link* prev, Link* next, const T& v) : Link{prev, next}, value(v) {}
    Node(Link* prev, Link* next, T&& v) : Link{prev, next}, value(std::move(v)) {}
  };

  Link head_{};  // head_.next = first payload node (or &tail_ when empty)
  Link tail_{};  // tail_.prev = last payload node (or &head_ when empty)
  std::size_t size_ = 0;

  void check_nonempty() const {
    if (size_ == 0) throw std::out_of_range("LinkedList: front/back on empty list");
  }

  void link_back(Link* n) {
    n->prev = tail_.prev;
    n->next = &tail_;
    tail_.prev->next = n;
    tail_.prev = n;
    ++size_;
  }

  // Transfer the payload region of `other` into `this` (other must be empty
  // afterwards and is reset to a consistent empty state). Sentinels stay with
  // their containers, so this never double-links across objects.
  void steal(LinkedList& other) noexcept {
    head_.next = other.head_.next;
    tail_.prev = other.tail_.prev;
    size_ = other.size_;
    if (head_.next) head_.next->prev = &head_;
    if (tail_.prev) tail_.prev->next = &tail_;
    other.head_.next = &other.tail_;
    other.tail_.prev = &other.head_;
    other.size_ = 0;
  }

 public:
  class iterator {
    Link* ptr_;
    friend class LinkedList;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit iterator(Link* p) : ptr_(p) {}
    T& operator*() const { return static_cast<Node*>(ptr_)->value; }
    T* operator->() const { return &static_cast<Node*>(ptr_)->value; }
    iterator& operator++() { ptr_ = ptr_->next; return *this; }
    iterator operator++(int) { iterator t(*this); ++(*this); return t; }
    iterator& operator--() { ptr_ = ptr_->prev; return *this; }
    iterator operator--(int) { iterator t(*this); --(*this); return t; }
    bool operator==(const iterator& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const iterator& o) const { return ptr_ != o.ptr_; }
  };

  class const_iterator {
    const Link* ptr_;
    friend class LinkedList;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    explicit const_iterator(const Link* p) : ptr_(p) {}
    const T& operator*() const { return static_cast<const Node*>(ptr_)->value; }
    const T* operator->() const { return &static_cast<const Node*>(ptr_)->value; }
    const_iterator& operator++() { ptr_ = ptr_->next; return *this; }
    const_iterator operator++(int) { const_iterator t(*this); ++(*this); return t; }
    const_iterator& operator--() { ptr_ = ptr_->prev; return *this; }
    const_iterator operator--(int) { const_iterator t(*this); --(*this); return t; }
    bool operator==(const const_iterator& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const const_iterator& o) const { return ptr_ != o.ptr_; }
  };

  LinkedList() {
    head_.next = &tail_;
    tail_.prev = &head_;
  }

  LinkedList(const LinkedList& other) : LinkedList() {
    for (const T& v : other) push_back(v);
  }
  LinkedList(LinkedList&& other) noexcept { steal(other); }
  LinkedList& operator=(const LinkedList& other) {
    if (this != &other) {
      LinkedList tmp(other);
      swap(tmp);
    }
    return *this;
  }
  LinkedList& operator=(LinkedList&& other) noexcept {
    if (this != &other) {
      clear();
      steal(other);
    }
    return *this;
  }
  ~LinkedList() { clear(); }

  // Relinks payloads between two live containers (empty/non-empty safe).
  void swap(LinkedList& other) noexcept {
    LinkedList tmp;
    tmp.steal(*this);
    this->steal(other);
    other.steal(tmp);
  }

  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  T& front() { check_nonempty(); return *begin(); }
  const T& front() const { check_nonempty(); return *begin(); }
  T& back() { check_nonempty(); return *(--end()); }
  const T& back() const { check_nonempty(); return *(--end()); }

  void push_back(const T& value) { link_back(new Node(&tail_, &tail_, value)); }
  void push_back(T&& value) { link_back(new Node(&tail_, &tail_, std::move(value))); }
  void push_front(const T& value) {
    Node* n = new Node(&head_, head_.next, value);
    head_.next->prev = n;
    head_.next = n;
    ++size_;
  }
  void push_front(T&& value) {
    Node* n = new Node(&head_, head_.next, std::move(value));
    head_.next->prev = n;
    head_.next = n;
    ++size_;
  }
  void pop_front() { if (!empty()) erase(begin()); }
  void pop_back() { if (!empty()) erase(--end()); }

  void clear() {
    Link* cur = head_.next;
    while (cur != &tail_) {
      Link* next = cur->next;
      delete static_cast<Node*>(cur);
      cur = next;
    }
    head_.next = &tail_;
    tail_.prev = &head_;
    size_ = 0;
  }

  // O(1) erase without searching. Returns the iterator to the successor
  // (end() when the last node was erased) so the id-om `it = erase(it)` works.
  iterator erase(iterator pos) {
    Link* n = pos.ptr_;
    if (n == &head_ || n == &tail_) return iterator(&tail_);
    Link* nx = n->next;
    n->prev->next = nx;
    nx->prev = n->prev;
    delete static_cast<Node*>(n);
    --size_;
    return iterator(nx);
  }

  // In-place pointer rewiring: every node swaps prev/next, then the head/tail
  // sentinels are re-attached to what used to be the last/first node.
  void reverse() {
    if (size_ < 2) return;
    Link* first = head_.next;
    Link* last = tail_.prev;
    for (Link* cur = first; cur != &tail_;) {
      Link* next = cur->next;
      std::swap(cur->prev, cur->next);
      cur = next;
    }
    head_.next = last;
    last->prev = &head_;
    tail_.prev = first;
    first->next = &tail_;
  }

  iterator begin() { return iterator(head_.next); }
  iterator end() { return iterator(&tail_); }
  const_iterator begin() const { return const_iterator(head_.next); }
  const_iterator end() const { return const_iterator(&tail_); }
};

#endif  // EXERCISE38_LINKED_LIST_H_