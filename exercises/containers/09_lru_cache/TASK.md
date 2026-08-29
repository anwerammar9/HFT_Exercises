# Exercise 09 — LRU Cache (Task)

## The problem (in plain words)

A fixed-capacity cache: `get(key)`/`put(key, value)` must both be **O(1)**, and
when the cache is full a `put` must evict the **least-recently-used** entry —
the one that has not been touched for the longest time. "Touched" means both
`get` and `put`, so the recency order is driven by *access*, not by insert
order. The classic shape is a hash map (O(1) lookup) + a doubly-linked recency
list (O(1) touch/evict).

## Requirements (what the tests check)

1. `get(key)`: returns `std::nullopt` on a miss; on a hit returns the value
   **and refreshes that key's recency** (moves it to the most-recently-used
   end).
2. `put(key, value)`: inserts a new key **or** updates an existing one in
   place; either way the key becomes most-recently-used.
3. When full, `put` evicts the **least-recently-used** key — explicitly the
   **untouched** entry, not merely the oldest-inserted (the tests distinguish
   these two).
4. `size()` and `capacity()` report correctly.
5. A large-`N` smoke test guards against accidentally O(n) behaviour — every
   operation must stay O(1)/O(capacity) bound, not scan.

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

The stub lives in `include/lru_cache.h` (a template) — implement inline,
replacing the `TODO(anwer)` bodies.

## How to think about it (suggested design)

- Two plain containers: `std::list<std::pair<K, V>> order_` (back = MRU,
  front = LRU) and `std::unordered_map<K, iterator>` `map_` (key → its position
  in the list).
- `get(key)`: find the iterator, copy the value out, `splice` the node to the
  back (or `erase`+push) — O(1).
- `put(key, v)`: if present, update the value and move to the back; if absent,
  evict the front when `size() == capacity()`, then push_back.
- Works for any `K`/`V` — no type tricks, just the two containers.
- Single-threaded by design.

## Make it harder (optional — not covered by the tests)

- **`get_or_emplace`:** a single call that returns an existing value or
  constructs + inserts, without doing two lookups.
- **Manual evict:** `evict(key)` forcing a key out early (useful for "symbol
  delisted" events).
- **Capacity that can change:** `set_capacity(n)` that evicts down to `n`.
- **TTL entries:** store `(value, expires_at)` and lazily drop expired entries
  on `get`/`put`, plus an `expire_all()` sweep.
- **Hybrid LFU/LRU:** evict cold-but-recent entries after a size threshold — a
  mini **ARC/w-TinyLFU** experiment on top of this structure.

## Files

- Stub: `include/lru_cache.h` (template — implement inline)
- Tests: `test/test_lru_cache.cpp`
- Reference: `SOLUTION.md`