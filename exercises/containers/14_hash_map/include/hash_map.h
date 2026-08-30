#ifndef EXERCISE24_HASH_MAP_H_
#define EXERCISE24_HASH_MAP_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <new>
#include <stdexcept>
#include <utility>

// Open-addressing hash map: a from-scratch std::unordered_map.
//
// Contract:
//   - insert(kv): true if a NEW key; false + existing position if present.
//   - operator[](k): reference to the mapped value (default-constructing when
//     absent); at(k) throws std::out_of_range on a miss.
//   - find(k): iterator or end(); erase(k): true iff present (slot becomes a
//     tombstone so probing still terminates); clear() empties but retains
//     storage.
//   - reserve(n): guarantees bucket_count() >= n before the next insert.
//   - begin()/end(): iterate ONLY filled slots, in undefined order.
//
// Implementation: linear probing, power-of-two buckets, 2x growth when
// load_factor(size_/bucket_count_) hits kMaxLoad; tombstones for erase.
// Single-threaded by design.
//
// TODO(anwer): implement the real table (see SOLUTION.md). The stub behaves as
// an EMPTY map and throws std::logic_error on inserts so tests run RED without
// dereferencing end() iterators.
template <typename K, typename V, typename H = std::hash<K>>
class HashMap {
 public:
  using key_type = K;
  using mapped_type = V;
  using value_type = std::pair<const K, V>;
  using size_type = std::size_t;
  using hasher = H;

  class Iterator;

  HashMap() = default;
  explicit HashMap(size_type capacity_hint);
  HashMap(const HashMap&);
  HashMap(HashMap&&) noexcept;
  HashMap& operator=(const HashMap&);
  HashMap& operator=(HashMap&&) noexcept;
  ~HashMap();

  size_type size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  size_type bucket_count() const noexcept { return capacity_; }

  std::pair<Iterator, bool> insert(const value_type& kv);
  V& operator[](const K& key);
  V& at(const K& key);
  const V& at(const K& key) const;
  bool contains(const K& key) const;
  Iterator find(const K& key);
  bool erase(const K& key);
  void clear() noexcept;
  void reserve(size_type n);

  Iterator begin() noexcept;
  Iterator end() noexcept;

  static constexpr double kMaxLoad = 0.7;

 private:
  enum class Slot : std::uint8_t { kEmpty, kFilled, kTombstone };

  struct Node {
    Slot slot = Slot::kEmpty;
    union U {
      value_type kv;
      char byte;
      U() : byte(0) {}
      ~U() {}
    } u;

    Node() = default;
    ~Node() { if (slot == Slot::kFilled) u.kv.~value_type(); }
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
  };

  Node* find_node(const K& key) noexcept;
  const Node* find_node(const K& key) const noexcept;
  void grow(size_type new_cap);
  static Node* alloc_empty(size_type cap);
  static void destroy_table(Node* t, size_type cap) noexcept;

  H hash_;
  Node* nodes_ = nullptr;
  size_type capacity_ = 0;
  size_type size_ = 0;
  size_type tombstones_ = 0;

  static constexpr size_type kInitialCapacity = 8;
};

// ---------------------------------------------------------------------------
// Iterator
// ---------------------------------------------------------------------------

template <typename K, typename V, typename H>
class HashMap<K, V, H>::Iterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = typename HashMap::value_type;
  using pointer = value_type*;
  using reference = value_type&;

  reference operator*() const { return cur_->u.kv; }
  pointer operator->() const { return &cur_->u.kv; }
  Iterator& operator++() {
    Node* stop = owner_->nodes_ + owner_->capacity_;
    while (++cur_ != stop) {
      if (cur_->slot == Slot::kFilled) return *this;
    }
    cur_ = nullptr;
    return *this;
  }

  friend bool operator==(const Iterator& a, const Iterator& b) {
    return a.cur_ == b.cur_;
  }
  friend bool operator!=(const Iterator& a, const Iterator& b) {
    return a.cur_ != b.cur_;
  }

 private:
  friend class HashMap;
  Iterator(HashMap* owner, Node* cur) : owner_(owner), cur_(cur) {}
  HashMap* owner_;
  Node* cur_;
};

// ---------------------------------------------------------------------------
// Stub implementation (TODO: replace with the real thing from SOLUTION.md)
// ---------------------------------------------------------------------------

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Node*
HashMap<K, V, H>::alloc_empty(size_type cap) {
  Node* t = static_cast<Node*>(::operator new(sizeof(Node) * cap));
  for (size_type i = 0; i < cap; ++i) {
    ::new (static_cast<void*>(t + i)) Node;
  }
  return t;
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::destroy_table(Node* t, size_type cap) noexcept {
  for (size_type i = 0; i < cap; ++i) {
    if (t[i].slot == Slot::kFilled) {
      t[i].u.kv.~value_type();
      t[i].slot = Slot::kEmpty;
    }
    t[i].~Node();
  }
  ::operator delete(t);
}

template <typename K, typename V, typename H>
HashMap<K, V, H>::HashMap(size_type capacity_hint) : capacity_(capacity_hint) {}

template <typename K, typename V, typename H>
HashMap<K, V, H>::HashMap(const HashMap& o) : hash_(o.hash_) {
  (void)o;  // TODO(anwer): deep-copy filled pairs
}

template <typename K, typename V, typename H>
HashMap<K, V, H>::HashMap(HashMap&& o) noexcept
    : hash_(std::move(o.hash_)), nodes_(o.nodes_), capacity_(o.capacity_),
      size_(o.size_), tombstones_(o.tombstones_) {
  o.nodes_ = nullptr;
  o.capacity_ = o.size_ = o.tombstones_ = 0;
}

template <typename K, typename V, typename H>
HashMap<K, V, H>& HashMap<K, V, H>::operator=(const HashMap& o) {
  if (this != &o) {
    HashMap tmp(o);
    *this = std::move(tmp);
  }
  return *this;
}

template <typename K, typename V, typename H>
HashMap<K, V, H>& HashMap<K, V, H>::operator=(HashMap&& o) noexcept {
  if (this != &o) {
    hash_ = std::move(o.hash_);
    destroy_table(nodes_, capacity_);
    nodes_ = o.nodes_;
    capacity_ = o.capacity_;
    size_ = o.size_;
    tombstones_ = o.tombstones_;
    o.nodes_ = nullptr;
    o.capacity_ = o.size_ = o.tombstones_ = 0;
  }
  return *this;
}

template <typename K, typename V, typename H>
HashMap<K, V, H>::~HashMap() {
  destroy_table(nodes_, capacity_);
}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Node*
HashMap<K, V, H>::find_node(const K& /*key*/) noexcept {
  return nullptr;  // TODO(anwer): linear probe
}

template <typename K, typename V, typename H>
const typename HashMap<K, V, H>::Node*
HashMap<K, V, H>::find_node(const K& /*key*/) const noexcept {
  return nullptr;  // TODO(anwer): linear probe
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::grow(size_type /*new_cap*/) {
  throw std::logic_error("not implemented");
}

template <typename K, typename V, typename H>
std::pair<typename HashMap<K, V, H>::Iterator, bool>
HashMap<K, V, H>::insert(const value_type& kv) {
  (void)kv;
  throw std::logic_error("not implemented");  // TODO(anwer): probe + tombstone
}

template <typename K, typename V, typename H>
V& HashMap<K, V, H>::operator[](const K& key) {
  (void)key;
  throw std::logic_error("not implemented");
}

template <typename K, typename V, typename H>
V& HashMap<K, V, H>::at(const K& /*key*/) {
  throw std::out_of_range("HashMap::at");
}

template <typename K, typename V, typename H>
const V& HashMap<K, V, H>::at(const K& /*key*/) const {
  throw std::out_of_range("HashMap::at");
}

template <typename K, typename V, typename H>
bool HashMap<K, V, H>::contains(const K& /*key*/) const {
  return false;  // TODO(anwer): find_node != nullptr
}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::find(const K& /*key*/) {
  return end();  // TODO(anwer): probe + iterator-to-node
}

template <typename K, typename V, typename H>
bool HashMap<K, V, H>::erase(const K& /*key*/) {
  return false;  // TODO(anwer): destroy pair + tombstone
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::clear() noexcept {
  size_ = 0;
  tombstones_ = 0;
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::reserve(size_type /*n*/) {}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::begin() noexcept {
  return end();  // TODO(anwer): scan to first kFilled slot
}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::end() noexcept {
  return Iterator(this, nullptr);
}

#endif  // EXERCISE24_HASH_MAP_H_