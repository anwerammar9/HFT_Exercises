# Exercise containers/04_hash_map (ex24) — Open-Addressing Hash Map (Reference Solution)

**What you implement:** a from-scratch `std::unordered_map` using open
addressing (linear probing), a power-of-two bucket count, tombstones for
erasure, and doubling growth at `load = size / capacity = kMaxLoad (0.7)`.

**Approach**
- `Node` packs a `pmr small state` + a union holding `value_type kv`
  (constructed only while `slot == kFilled`; empty slots hold a filler char so
  `K` needn't be default-constructible). Tables are raw `::operator new[]` with
  placement `Node`/`kv` and manual destroys (`alloc_empty`/`destroy_table`).
- `hash(key) & (capacity - 1)`; probe linearly, stopping at `kEmpty`.
- `insert`: on load-exceeded grows 2x; records the first tombstone in the
  cluster and reuses it on insert (bumps `size_`, drops `tombstones_`);
  `{Iterator, true}` when keys are new, `{Iterator(pos), false}` otherwise;
  a fully-tombstoned saturated table grows once and retries.
- `erase`: destroys the pair, marks `kTombstone` (so probing still terminates),
  decrements `size_` / increments `tombstones_` (`size()` stays == live count).
- `operator[]` defers to `insert(value_type(key, V{}))`, `at()` throws
  `out_of_range` on miss; `find`/`contains` probe via `find_node`.
- `begin()` scans to the first `kFilled` slot; `end()` is `Iterator(this,
  nullptr)`; `operator++` skips non-filled slots.
- `clear()` empties slots but **retains storage and bucket_count()** (the
  reinsert-after-clear test depends on it).
- Copy ctor deep-copies filled pairs; move ctor steals the table.

## Reference API — `include/hash_map.h`
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
//   - insert(kv): true if a NEW key was inserted, false + existing position if
//     the key was already present.
//   - operator[](k): returns a reference to the mapped value, default-
//     constructing it if the key is absent.
//   - at(k): like operator[] but throws std::out_of_range on a miss.
//   - find(k): iterator to the element, or end().
//   - erase(k): true iff the key was present; the slot becomes a tombstone so
//     later probes of the same cluster still terminate.
//   - reserve(n): guarantees bucket_count() >= n before the next insert.
//   - begin()/end(): iterate only FILLED slots, in undefined order.
//
// Implementation: linear probing, power-of-two bucket count, grow x2 when
// load_factor (size_ / bucket_count_) reaches kMaxLoad; erase leaves a
// tombstone; an insert that reuses a tombstone bumps size_ and decrements the
// tombstone count (size_ always equals the number of live elements).
// Single-threaded by design (as the STL is).

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

  // A node owns its pair only while FILLED (K may not be default-constructible;
  // empty slots must not hold a value_type). Placement-new / manual destroy.
  struct Node {
    Slot slot = Slot::kEmpty;
    union U {
      value_type kv;
      char byte;
      U() : byte(0) {}
      ~U() {}
    } u;  // u.kv exists only while slot == Filled (constructed via placement new)

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
// Implementation
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
HashMap<K, V, H>::HashMap(size_type capacity_hint) {
  if (capacity_hint > 0) {
    size_type cap = kInitialCapacity;
    while (cap < capacity_hint) cap *= 2;
    nodes_ = alloc_empty(cap);
    capacity_ = cap;
  }
}

template <typename K, typename V, typename H>
HashMap<K, V, H>::HashMap(const HashMap& o) : hash_(o.hash_) {
  if (o.capacity_ != 0) {
    nodes_ = alloc_empty(o.capacity_);
    capacity_ = o.capacity_;
    for (size_type i = 0; i < o.capacity_; ++i) {
      if (o.nodes_[i].slot == Slot::kFilled) {
        ::new (static_cast<void*>(&nodes_[i].u.kv)) value_type(o.nodes_[i].u.kv);
        nodes_[i].slot = Slot::kFilled;
      }
    }
    size_ = o.size_;
    tombstones_ = 0;
  }
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
HashMap<K, V, H>::find_node(const K& key) noexcept {
  if (capacity_ == 0) return nullptr;
  size_type idx = hash_(key) & (capacity_ - 1);
  for (size_type probes = 0; probes < capacity_; ++probes) {
    Node& n = nodes_[idx];
    if (n.slot == Slot::kEmpty) return nullptr;
    if (n.slot == Slot::kFilled && n.u.kv.first == key) return &n;
    idx = (idx + 1) & (capacity_ - 1);
  }
  return nullptr;
}

template <typename K, typename V, typename H>
const typename HashMap<K, V, H>::Node*
HashMap<K, V, H>::find_node(const K& key) const noexcept {
  if (capacity_ == 0) return nullptr;
  size_type idx = hash_(key) & (capacity_ - 1);
  for (size_type probes = 0; probes < capacity_; ++probes) {
    const Node& n = nodes_[idx];
    if (n.slot == Slot::kEmpty) return nullptr;
    if (n.slot == Slot::kFilled && n.u.kv.first == key) return &n;
    idx = (idx + 1) & (capacity_ - 1);
  }
  return nullptr;
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::grow(size_type new_cap) {
  if (new_cap < kInitialCapacity) new_cap = kInitialCapacity;
  Node* t = alloc_empty(new_cap);
  if (nodes_ != nullptr) {
    for (size_type i = 0; i < capacity_; ++i) {
      if (nodes_[i].slot != Slot::kFilled) continue;
      size_type idx = hash_(nodes_[i].u.kv.first) & (new_cap - 1);
      while (t[idx].slot != Slot::kEmpty) idx = (idx + 1) & (new_cap - 1);
      ::new (static_cast<void*>(&t[idx].u.kv))
          value_type(std::move(nodes_[i].u.kv));
      t[idx].slot = Slot::kFilled;
    }
    destroy_table(nodes_, capacity_);
  }
  nodes_ = t;
  capacity_ = new_cap;
  tombstones_ = 0;
}

template <typename K, typename V, typename H>
std::pair<typename HashMap<K, V, H>::Iterator, bool>
HashMap<K, V, H>::insert(const value_type& kv) {
  if (capacity_ == 0 || static_cast<double>(size_ + 1) > capacity_ * kMaxLoad) {
    grow(capacity_ == 0 ? kInitialCapacity : capacity_ * 2);
  }
  size_type idx = hash_(kv.first) & (capacity_ - 1);
  size_type tomb = static_cast<size_type>(-1);
  for (size_type probes = 0; probes < capacity_; ++probes) {
    Node& n = nodes_[idx];
    if (n.slot == Slot::kFilled && n.u.kv.first == kv.first) {
      return {Iterator(this, &nodes_[idx]), false};
    }
    if (n.slot == Slot::kTombstone && tomb == static_cast<size_type>(-1)) {
      tomb = idx;
    }
    if (n.slot == Slot::kEmpty) {
      size_type ins = tomb == static_cast<size_type>(-1) ? idx : tomb;
      Node& i = nodes_[ins];
      if (i.slot == Slot::kTombstone) {
        --tombstones_;
      }
      ++size_;
      ::new (static_cast<void*>(&i.u.kv)) value_type(kv);
      i.slot = Slot::kFilled;
      return {Iterator(this, &nodes_[ins]), true};
    }
    idx = (idx + 1) & (capacity_ - 1);
  }
  // Fully saturated (possible with many tombstones): grow and retry once.
  grow(capacity_ * 2);
  return insert(kv);
}

template <typename K, typename V, typename H>
V& HashMap<K, V, H>::operator[](const K& key) {
  return insert(value_type(key, V{})).first->second;
}

template <typename K, typename V, typename H>
V& HashMap<K, V, H>::at(const K& key) {
  Node* n = find_node(key);
  if (n == nullptr) throw std::out_of_range("HashMap::at");
  return n->u.kv.second;
}

template <typename K, typename V, typename H>
const V& HashMap<K, V, H>::at(const K& key) const {
  const Node* n = find_node(key);
  if (n == nullptr) throw std::out_of_range("HashMap::at");
  return n->u.kv.second;
}

template <typename K, typename V, typename H>
bool HashMap<K, V, H>::contains(const K& key) const {
  return find_node(key) != nullptr;
}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::find(const K& key) {
  Node* n = find_node(key);
  return n != nullptr ? Iterator(this, n) : end();
}

template <typename K, typename V, typename H>
bool HashMap<K, V, H>::erase(const K& key) {
  Node* n = find_node(key);
  if (n == nullptr) return false;
  n->u.kv.~value_type();
  n->slot = Slot::kTombstone;
  ++tombstones_;
  --size_;
  return true;
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::clear() noexcept {
  if (nodes_ != nullptr) {
    for (size_type i = 0; i < capacity_; ++i) {
      if (nodes_[i].slot == Slot::kFilled) {
        nodes_[i].u.kv.~value_type();
        nodes_[i].slot = Slot::kEmpty;
      }
    }
    // Storage (and thus bucket_count()) is retained on purpose.
  }
  size_ = 0;
  tombstones_ = 0;
}

template <typename K, typename V, typename H>
void HashMap<K, V, H>::reserve(size_type n) {
  if (n <= capacity_) return;
  size_type cap = capacity_ == 0 ? kInitialCapacity : capacity_;
  while (cap < n) cap *= 2;
  grow(cap);
}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::begin() noexcept {
  if (capacity_ == 0) return end();
  Node* stop = nodes_ + capacity_;
  for (Node* p = nodes_; p != stop; ++p) {
    if (p->slot == Slot::kFilled) return Iterator(this, p);
  }
  return end();
}

template <typename K, typename V, typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::end() noexcept {
  return Iterator(this, nullptr);
}

#endif  // EXERCISE24_HASH_MAP_H_
## Reference TU — `src/hash_map.cpp` (explicit instantiation)
#include "hash_map.h"

#include <string>

// Explicit instantiations: guarantees the header (stubs included) really
// compiles for both POD- and std-types.
template class HashMap<int, int>;
template class HashMap<std::string, std::string>;
template class HashMap<int, std::string>;