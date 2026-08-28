# Exercise 09 — LRU Cache (Reference Solution)

**What you implement:** a fixed-capacity cache with O(1) get/put and O(1) cheap
recency maintenance, keyed by "most-recently-used" eviction.

**Approach**
- `std::unordered_map<K, list::iterator>` + `std::list<pair<K,V>>` where
  `front` = LRU and `back` = MRU (the standard "map of iterators" trick).
- `get(key)`: map lookup; on hit `splice` the entry to the MRU end and return
  its value; `nullopt` on miss.
- `put(key, value)`: existing key → update value in place AND `splice` to MRU;
  new key → if `size() == capacity_` evict `order_.front()` (map erase +
  list pop_front), then `emplace_back` and store the iterator.
- `size()` is just `map_.size()`; `capacity()` is fixed.

## Reference API — `include/lru_cache.h`
#ifndef EXERCISE09_LRU_CACHE_H_
#define EXERCISE09_LRU_CACHE_H_

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

// Fixed-capacity LRU cache: hash map + intrusive recency list.
//
// Contract:
//   - `get(key)`: returns std::nullopt when absent; when present, returns the
//     value AND refreshes the key's recency (moves it to the MRU end).
//   - `put(key, value)`: inserts, or updates in place; either way the key
//     becomes the MRU entry. When the cache is full, the LEAST-recently-used
//     key is evicted.
//   - Single-threaded.
//
// Implementation: unordered_map for O(1) lookup, a std::list serving as the
// intrusive recency order (back = MRU, front = LRU), and map values storing
// iterators into the list for O(1) moves/evictions.

template <typename K, typename V>
class LruCache {
 public:
  explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

  std::optional<V> get(const K& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    // Move to MRU (back).
    order_.splice(order_.end(), order_, it->second);
    return it->second->second;
  }

  void put(const K& key, V value) {
    auto it = map_.find(key);
    if (it != map_.end()) {
      it->second->second = std::move(value);
      order_.splice(order_.end(), order_, it->second);  // refresh recency
      return;
    }
    if (size() == capacity_) evict_lru();
    order_.emplace_back(key, std::move(value));
    auto lit = std::prev(order_.end());
    map_[key] = lit;
  }

  std::size_t size() const { return map_.size(); }
  std::size_t capacity() const { return capacity_; }

 private:
  void evict_lru() {
    const K& lru_key = order_.front().first;
    map_.erase(lru_key);
    order_.pop_front();
  }

  std::size_t capacity_{0};
  std::list<std::pair<K, V>> order_;
  std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};

#endif  // EXERCISE09_LRU_CACHE_H_

## Reference TU — `src/lru_cache.cpp` (explicit instantiation)
#include "lru_cache.h"

#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers).
// Keeping this TU pins the instantiations exercised by the unit tests.

template class LruCache<std::string, int>;
template class LruCache<int, std::string>;