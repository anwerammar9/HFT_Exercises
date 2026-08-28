# Exercise 09 — LRU Cache (Task)

## Problem
A fixed-capacity cache with O(1) get/put and O(1) recency maintenance, evicting
the **least-recently-used** entry when full.

## Requirements (what the tests check)
1. `get(key)`: `std::nullopt` on a miss; on a hit returns the value **and**
   refreshes the key's recency (moves it to the MRU end).
2. `put(key, value)`: inserts a new key or updates an existing one in place;
   either way the key becomes MRU.
3. When full, `put` evicts the **least-recently-used** key — explicitly the
   *untouched* entry, not merely the oldest-inserted.
4. `size()` and `capacity()` report correctly.
5. A large-N smoke test guards against accidental O(n) behaviour.

## Public API
```cpp
template <typename K, typename V>
class LruCache {
  explicit LruCache(std::size_t capacity);
  std::optional<V> get(const K& key);
  void put(const K& key, V value);
  std::size_t size() const;
  std::size_t capacity() const;
};
```

## Design notes
`std::list<std::pair<K,V>> order_` (back = MRU, front = LRU) +
`std::unordered_map<K, iterator> map_`. get/put touch the list in O(1).
Single-threaded.

## Files
- Stub: `include/lru_cache.h` (template — implement inline)
- Tests: `test/test_lru_cache.cpp`
- Reference: `SOLUTION.md`