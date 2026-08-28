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
//   - `get(key)`: std::nullopt when absent; on hit returns the value AND
//     refreshes the key's recency (moves it to the MRU end).
//   - `put(key, value)`: inserts or updates in place; either way the key
//     becomes MRU. When full, the LEAST-recently-used key is evicted.
//   - Single-threaded.
//
// TODO(anwer): implement on `order_` (back = MRU, front = LRU) + `map_` of
// key -> iterator (see SOLUTION.md).

template <typename K, typename V>
class LruCache {
 public:
  explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

  std::optional<V> get(const K& /*key*/) { return std::nullopt; }

  void put(const K& /*key*/, V /*value*/) {}

  std::size_t size() const { return map_.size(); }
  std::size_t capacity() const { return capacity_; }

 private:
  std::size_t capacity_{0};
  std::list<std::pair<K, V>> order_;
  std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};

#endif  // EXERCISE09_LRU_CACHE_H_