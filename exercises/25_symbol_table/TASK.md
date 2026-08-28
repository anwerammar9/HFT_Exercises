# Exercise 25 — Symbol Table (Task)

## Problem
A thread-safe, two-way symbol ⇄ id registry — the "symbol interning" table a
feed handler keeps so hot paths compare 4-byte ids instead of hashing strings.

## Requirements (what the tests check)
1. `intern(symbol)`: returns the existing id, or assigns the next **monotonic**
   id (0, 1, 2, …) and registers the string. **Ids are NEVER reused**, even
   after erase. Case-sensitive exact match.
2. `find(symbol)`: the id or `nullopt`.
3. `lookup(id)`: a non-owning `string_view` that stays **valid until
   `clear()`** (low-latency memory retention — interned strings are never freed
   early), or `nullopt` for unknown/erased ids.
4. `erase_symbol`/`erase_id`: remove one direction; `false` when absent. The
   storage backing earlier string_views stays alive (retention), but
   `lookup()` must report `nullopt` afterwards.
5. `size()` = number of **currently registered** (live) symbols — not slots
   used.
6. `clear()` drops everything; **ids restart from 0** afterwards.
7. Thread safety: all public methods internally synchronized (the provided
   mutex); a stress test interns `threads × 1000` distinct symbols concurrently
   and checks interning uniqueness + both directions (`tsan;stress`).

## Public API
```cpp
class SymbolTable {
  std::int32_t intern(std::string_view symbol);
  std::optional<std::int32_t> find(std::string_view symbol) const;
  std::optional<std::string_view> lookup(std::int32_t id) const;
  bool erase_symbol(std::string_view symbol);
  bool erase_id(std::int32_t id);
  std::size_t size() const noexcept;
  void clear();
};
```

## Design notes
Given members: `symbol_to_id_` (unordered_map), `id_to_symbol_` (vector,
index == id, **retained on erase**), `live_` (vector of flags, index == id).
Intern appends `id = id_to_symbol_.size()`. Decide whether the mutex-only read
path is enough or a lock-free read path (RCU via atomic shared_ptr) is worth it.

## Files
- Stub: `src/symbol_table.cpp`
- Tests: `test/test_symbol_table.cpp`
- Reference: `SOLUTION.md`