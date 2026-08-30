# Exercise 24 — Open-Addressing Hash Map (Task)

## The problem (in plain words)

Re-implement `std::unordered_map` with **open addressing**: every key lives
directly in a fixed array of slots, probed **linearly** from its hash bucket;
deleted slots become **tombstones** so probing still terminates; and the table
**doubles** when a load factor of 0.7 is hit. This is the standard low-latency
hash table shape — and the tombstone handling, growth, and iterator skipping
are exactly the parts people get subtly wrong.

## Requirements (what the tests check)

1. `insert(kv)`: returns `{Iterator, true}` for a NEW key, or
   `{Iterator, false}` positioned at the existing entry when present.
2. `operator[](k)`: reference to the mapped value, **value-inserting** when
   absent. `at(k)`: reference (const and non-const), **throws
   `std::out_of_range`** on a miss.
3. `find(k)`/`contains(k)`: locate via linear probing; `find` returns `end()`
   when absent.
4. `erase(k)`: `true` iff present. The slot becomes a **tombstone** (so
   probing still terminates) and `size()` counts only live entries; a later
   insert **reuses a tombstone** (the tombstone count drops).
5. Growth: **doubling (×2)** whenever `load = size()/bucket_count()` would hit
   `kMaxLoad = 0.7`; `reserve(n)` guarantees `bucket_count() >= n` before the
   next insert.
6. `clear()` empties all slots but **retains storage and `bucket_count()`**
   (reinserting after `clear()` works).
7. `begin()`/`end()` iterate **only filled slots**, in **undefined order** (a
   forward iterator that skips empties and tombstones).
8. Copy ctor deep-copies filled pairs; move ctor steals the table (source left
   empty).
9. A `BadHash` collision-storm test forces heavy probing without breaking —
   correctness must not depend on a good hash.
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

The stub lives in `include/hash_map.h` (a template) — implement inline. The
`Node`/`Slot` storage, union-based value slot, and the iterator skeleton are
already laid out in the header; you fill in probing, grow, insert/erase, and
iteration skipping.

## How to think about it (suggested design)

- `nodes_` is a flat array of `Node{Slot tag; union U value;}`; `Slot` is
  `kEmpty` / `kFilled` / `kTombstone`.
- **Probe start** at `hash(key) & (capacity_ − 1)` (power-of-two mask, no `%`);
  walk forward (wrapping at the end) past tombstones/others until `kEmpty` or a
  match.
- **insert:** find a free-or-tombstone slot; if the key is already present,
  return `{it, false}`; when the load would exceed `kMaxLoad`, `grow()` first.
- **grow:** allocate a fresh array, re-insert every filled pair (tombstones are
  dropped), destroy the old one.
- **erase:** mark `kTombstone`, decrement `size_`, increment `tombstones_`;
  reuse of a tombstone decrements it.
- Iterator `++` scans forward to the next `kFilled` slot — already sketched in
  the header.

## Make it harder (optional — not covered by the tests)

- **`emplace`:** construct the pair in place with forwarded args, reusing any
  tombstone, and return the same `{Iterator, bool}` shape as `insert`.
- **Robust probing under `BadHash`:** compare *average probe length* for
  linear vs. another scheme (e.g. double hashing) on the collision-storm test.
- **`load_factor()` and `max_load_factor()`:** report and set the growth
  threshold (also exercises `reserve` interplay).
- **Iterators that survive erase:** define (and document) whether an iterator
  to a key stays valid after unrelated insert/erase — then enforce it.
- **Robin Hood hashing:** swap elements to bound worst-case probe length, and
  measure the improvement.

## Files

- Stub: `include/hash_map.h` (template — implement inline)
- Tests: `test/test_hash_map.cpp`
- Reference: `SOLUTION.md`