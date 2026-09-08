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
// Implementation: the head_/tail_ sentinels are plain Links owned by the
// container; begin() is head_.next, end() is &tail_, so an empty list costs
// zero allocations. Move/swap relocate the payload region into the container's
// OWN sentinels — never cross-link another object's sentinels (they live inside
// the objects and would corrupt empty lists).

template <typename T>
class LinkedList {
 private:
  struct Link {
    Link* prev;
    Link* next;
    Link() noexcept : prev(this), next(this) {}
  };

  struct Node : Link {
    T value;
    explicit Node(const T& v) : value(v) {}
    explicit Node(T&& v) : value(std::move(v)) {}
  };

  void check_nonempty() const {
    if (size_ == 0) throw std::out_of_range("LinkedList::front/back on empty list");
  }

  template <typename U>
  void push_back_impl(U&& v) {
    Node* n = new Node(std::forward<U>(v));
    n->prev = tail_.prev;
    n->next = &tail_;
    tail_.prev->next = n;
    tail_.prev = n;
    ++size_;
  }

  template <typename U>
  void push_front_impl(U&& v) {
    Node* n = new Node(std::forward<U>(v));
    n->next = head_.next;
    n->prev = &head_;
    head_.next->prev = n;
    head_.next = n;
    ++size_;
  }

  // Relocate src's whole payload region into *this (which must be empty),
  // then leave src empty. The sentinels never move — they stay with their own
  // container, so this never double-links across objects.
  void adopt_body(LinkedList& src) noexcept {
    if (src.head_.next == &src.tail_) return;
    head_.next = src.head_.next;
    head_.next->prev = &head_;
    tail_.prev = src.tail_.prev;
    tail_.prev->next = &tail_;
    size_ = src.size_;
    src.head_.next = &src.tail_;
    src.tail_.prev = &src.head_;
    src.size_ = 0;
  }

 public:
  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    iterator() noexcept : ptr_(nullptr) {}
    explicit iterator(Link* p) noexcept : ptr_(p) {}

    reference operator*() const { return static_cast<Node*>(ptr_)->value; }
    pointer operator->() const { return &(operator*()); }

    iterator& operator++() noexcept { ptr_ = ptr_->next; return *this; }
    iterator operator++(int) noexcept {
      iterator t(*this);
      ++(*this);
      return t;
    }
    iterator& operator--() noexcept { ptr_ = ptr_->prev; return *this; }
    iterator operator--(int) noexcept {
      iterator t(*this);
      --(*this);
      return t;
    }

    bool operator==(const iterator& o) const noexcept { return ptr_ == o.ptr_; }
    bool operator!=(const iterator& o) const noexcept { return ptr_ != o.ptr_; }

   private:
    friend class LinkedList;
    friend class const_iterator;
    Link* ptr_;
  };

  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    const_iterator() noexcept : ptr_(nullptr) {}
    explicit const_iterator(const Link* p) noexcept : ptr_(p) {}
    const_iterator(const iterator& it) noexcept : ptr_(it.ptr_) {}

    reference operator*() const { return static_cast<const Node*>(ptr_)->value; }
    pointer operator->() const { return &(operator*()); }

    const_iterator& operator++() noexcept { ptr_ = ptr_->next; return *this; }
    const_iterator operator++(int) noexcept {
      const_iterator t(*this);
      ++(*this);
      return t;
    }
    const_iterator& operator--() noexcept { ptr_ = ptr_->prev; return *this; }
    const_iterator operator--(int) noexcept {
      const_iterator t(*this);
      --(*this);
      return t;
    }

    bool operator==(const const_iterator& o) const noexcept {
      return ptr_ == o.ptr_;
    }
    bool operator!=(const const_iterator& o) const noexcept {
      return ptr_ != o.ptr_;
    }

   private:
    friend class LinkedList;
    const Link* ptr_;
  };

  LinkedList() {
    head_.next = &tail_;
    tail_.prev = &head_;
  }
  LinkedList(const LinkedList& other) {
    head_.next = &tail_;
    tail_.prev = &head_;
    for (const T& v : other) push_back(v);
  }
  LinkedList(LinkedList&& other) noexcept {
    head_.next = &tail_;
    tail_.prev = &head_;
    adopt_body(other);
  }
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
      adopt_body(other);
    }
    return *this;
  }
  ~LinkedList() { clear(); }

  void swap(LinkedList& other) noexcept {
    if (head_.next == &tail_) {
      adopt_body(other);
    } else if (other.head_.next == &other.tail_) {
      other.adopt_body(*this);
    } else {
      LinkedList tmp;
      tmp.adopt_body(*this);
      adopt_body(other);
      other.adopt_body(tmp);
    }
  }

  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  T& front() {
    check_nonempty();
    return static_cast<Node*>(head_.next)->value;
  }
  const T& front() const {
    check_nonempty();
    return static_cast<const Node*>(head_.next)->value;
  }
  T& back() {
    check_nonempty();
    return static_cast<Node*>(tail_.prev)->value;
  }
  const T& back() const {
    check_nonempty();
    return static_cast<const Node*>(tail_.prev)->value;
  }

  void push_back(const T& v) { push_back_impl(v); }
  void push_back(T&& v) { push_back_impl(std::move(v)); }
  void push_front(const T& v) { push_front_impl(v); }
  void push_front(T&& v) { push_front_impl(std::move(v)); }

  void pop_front() {
    if (empty()) return;
    Node* n = static_cast<Node*>(head_.next);
    head_.next = n->next;
    n->next->prev = &head_;
    delete n;
    --size_;
  }
  void pop_back() {
    if (empty()) return;
    Node* n = static_cast<Node*>(tail_.prev);
    n->prev->next = &tail_;
    tail_.prev = n->prev;
    delete n;
    --size_;
  }

  void clear() noexcept {
    Link* cur = head_.next;
    while (cur != &tail_) {
      Link* nxt = cur->next;
      delete static_cast<Node*>(cur);
      cur = nxt;
    }
    head_.next = &tail_;
    tail_.prev = &head_;
    size_ = 0;
  }

  // O(1) erase WITHOUT searching. Returns the iterator to the successor, so
  // the erase-loop idiom `it = l.erase(it);` works. Erasing end() is a no-op.
  iterator erase(iterator it) {
    if (it.ptr_ == &tail_) return end();
    Link* n = it.ptr_;
    Link* succ = n->next;
    n->prev->next = succ;
    succ->prev = n->prev;
    delete static_cast<Node*>(n);
    --size_;
    return iterator(succ);
  }

  // In-place pointer rewiring: every payload node swaps prev/next, then the
  // sentinels re-attach to the former last/first node. O(n), no allocation.
  void reverse() {
    if (head_.next == &tail_) return;
    Link* cur = head_.next;
    while (cur != &tail_) {
      std::swap(cur->prev, cur->next);
      cur = cur->prev;
    }
    Link* first = head_.next;
    Link* last = tail_.prev;
    head_.next = last;
    last->prev = &head_;
    tail_.prev = first;
    first->next = &tail_;
  }

  iterator begin() { return iterator(head_.next); }
  iterator end() { return iterator(&tail_); }
  const_iterator begin() const { return const_iterator(head_.next); }
  const_iterator end() const { return const_iterator(&tail_); }

 private:
  Link head_;
  Link tail_;
  std::size_t size_ = 0;
};

#endif  // EXERCISE38_LINKED_LIST_H_