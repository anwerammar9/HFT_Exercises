#ifndef EXERCISE38_LINKED_LIST_H_
#define EXERCISE38_LINKED_LIST_H_

#include <cstddef>
#include <iterator>
#include <stdexcept>

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
// always-empty sentinel list whose mutators are no-ops and whose iterators
// deref a static dummy, so every mutation/reverse/erase/copy assertion fails
// RED without allocating, dereferencing invalid memory, or crashing.

template <typename T>
class LinkedList {
 public:
  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

T& operator*() const {
      static T stub{};
      return stub;
    }
    T* operator->() const { return &(operator*()); }
    iterator& operator++() { return *this; }
    iterator operator++(int) { return *this; }
    iterator& operator--() { return *this; }
    iterator operator--(int) { return *this; }
    bool operator==(const iterator&) const { return true; }
    bool operator!=(const iterator&) const { return false; }
  };

  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    const T& operator*() const {
      static T stub{};
      return stub;
    }
    const T* operator->() const { return &(operator*()); }
    const_iterator& operator++() { return *this; }
    const_iterator operator++(int) { return *this; }
    const_iterator& operator--() { return *this; }
    const_iterator operator--(int) { return *this; }
    bool operator==(const const_iterator&) const { return true; }
    bool operator!=(const const_iterator&) const { return false; }
  };

  LinkedList() = default;
  LinkedList(const LinkedList&) = default;
  LinkedList(LinkedList&&) = default;
  LinkedList& operator=(const LinkedList&) = default;
  LinkedList& operator=(LinkedList&&) = default;
  ~LinkedList() = default;

  void swap(LinkedList&) noexcept {}

  std::size_t size() const noexcept { return 0; }
  bool empty() const noexcept { return true; }

  T& front() { throw std::logic_error("not implemented"); }
  const T& front() const { throw std::logic_error("not implemented"); }
  T& back() { throw std::logic_error("not implemented"); }
  const T& back() const { throw std::logic_error("not implemented"); }

  void push_back(const T&) {}
  void push_back(T&&) {}
  void push_front(const T&) {}
  void push_front(T&&) {}
  void pop_front() {}
  void pop_back() {}

  void clear() noexcept {}

  iterator erase(iterator) { return iterator{}; }
  void reverse() {}

  iterator begin() { return iterator{}; }
  iterator end() { return iterator{}; }
  const_iterator begin() const { return const_iterator{}; }
  const_iterator end() const { return const_iterator{}; }
};

#endif  // EXERCISE38_LINKED_LIST_H_