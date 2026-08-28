# Exercise 24 — Open-Addressing Hash Map (Task)

## Problem
A from-scratch `std::unordered_map`: **open addressing** with linear probing,
a power-of-two bucket count, **tombstones** for erasure, and doubling growth at
load 0.7.

## Requirements (what the tests check)
1. `insert(kv)`: returns `{Iterator, true}` for a NEW key, `{Iterator, false}`
   (at the existing position) when present.
2. `operator[](k)`: reference to the mapped value, value-inserting when absent.
   `at(k)`: reference; **throws `std::out_of_range`** on a miss.
3. `find(k)`/`contains(k)`: locate via linear probing (find returns `end()`
   when absent).
4. `erase(k)`: `true` iff present; the slot becomes a **tombstone** (so probing
   still terminates) and `size()` counts only live entries; a tombstone slot is
   reused by a later insert (tombstone count drops).
5. Growth: **doubling** (×2) when `load = size/bucket_count` hits
   `kMaxLoad (0.7)`; `reserve(n)` guarantees `bucket_count() >= n` before the
   next insert.
6. `clear()` empties all slots but **retains storage and bucket_count()**
   (reinsert-after-clear works).
7. `begin()`/`end()` iterate ONLY filled slots, in **undefined order**
   (forward iterator that skips empties/tombstones).
8. Copy ctor deep-copies filled pairs; move ctor steals the table.
9. A `BadHash` collision-storm test forces heavy probing without breaking.
10. `size()`, `empty()`, `bucket_count()` report correctly.

## Public API
```cpp
template <typename K, typename V, typename H = std::hash<K>>
class HashMap {
  // usings: key_type, mapped_type, value_type=pair<const K,V>, size_type, hasher
  HashMap();  HashMap(size_type capacity_hint);
  HashMap(const HashMap&);  HashMap(HashMap&&) noexcept;
  HashMap& operator=(const HashMap&);  HashMap& operator=(HashMap&&) noexcept;
  ~HashMap();
  size_type size() const;  bool empty() const;  size_type bucket_count() const;
  std::pair<Iterator, bool> insert(const value_type& kv);
  V& operator[](const K& key);  V& at(const K& key);  const V& at(const K&) const;
  bool contains(const K& key) const;  Iterator find(const K& key);
  bool erase(const K& key);  void clear() noexcept;  void reserve(size_type n);
  Iterator begin() noexcept;  Iterator end() noexcept;
  static constexpr double kMaxLoad = 0.7;
};
```

## Files
- Stub: `include/hash_map.h` (template — implement inline)
- Tests: `test/test_hash_map.cpp`
- Reference: `SOLUTION.md`