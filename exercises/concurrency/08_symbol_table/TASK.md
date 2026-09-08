# Exercise concurrency/08_symbol_table (ex25) — Symbol Table (Task)

## The problem (in plain words)

A feed handler compares symbols all day: `"EURUSD" vs "EURUSD"` means hashing
and comparing bytes on every message. The trick is **symbol interning**: give
each symbol a small integer id once, then compare 4-byte ids forever after. The
table must be **thread-safe**, **two-way** (symbol ⇄ id), and — the hard part —
the `string_view`s it returns must stay valid as long as the table lives
(low-latency memory *retention*: interned strings are never freed early).

## Requirements (what the tests check)

1. `intern(symbol)`: returns the existing id, or assigns the next **monotonic**
   id (0, 1, 2, …) and registers the string. **Ids are NEVER reused**, even
   after `erase_symbol` (holes are fine). Case-sensitive exact match.
2. `find(symbol)`: the id or `std::nullopt`.
3. `lookup(id)`: a **non-owning `string_view`** that stays **valid until
   `clear()`** — interned strings are never freed before then — or `nullopt`
   for unknown/erased ids.
4. `erase_symbol(symbol)` / `erase_id(id)`: remove one direction; `false` when
   absent. The storage backing earlier `string_view`s stays alive (retention),
   but `lookup(id)` must report `nullopt` for the erased id afterwards.
5. `size()` = number of **currently registered** (live) symbols — not slots
   used.
6. `clear()` drops everything; **ids restart from 0** afterwards.
7. **Thread safety:** all public methods internally synchronized (the provided
   mutex); a stress test interns `threads × 1000` distinct symbols
   concurrently and checks interning uniqueness + both directions
   (`tsan;stress`).

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

## How to think about it (suggested design)

- The header gives you the shape: `symbol_to_id_` (string → id),
  `id_to_symbol_` (vector, **index == id**, entries *retained* on erase), and
  `live_` (vector of flags, index == id, `1` while registered).
- `intern` appends with `id = id_to_symbol_.size()` — that's what makes ids
  monotonic and never-reused, and it makes `lookup(id)` O(1).
- **Retention:** `erase` clears `live_[id]` but leaves `id_to_symbol_[id]` in
  place, so a previously returned `string_view` never dangles and `lookup` can
  still know the id *existed* but is no longer live.
- **Thread safety:** every public method takes the provided `std::mutex`.
  Decide (and comment) whether a plain mutex-protected read path is enough, or
  whether a lock-free read path (e.g. RCU via atomic shared garbage) is worth
  the complexity.

## Make it harder (optional — not covered by the tests)

- **Stress the retention contract:** intern, take a `string_view`, erase,
  re-intern a *different* symbol on the same id — assert the old view still
  reads its original bytes and `lookup` reflects the new symbol.
- **Batch intern:** `intern_many(vector<string_view>)` under one lock
  (fewer contentions), returning the ids.
- **`stats()`:** live count, total ever-interned, in-use bytes of retained
  storage, and the current highest id — visibility a trading desk would want.
- **Move to a read-mostly path:** keep writes behind the mutex but let readers
  snapshot the `id_to_symbol_` vector into an immutable copy (COW) so reads
  need no lock — then measure the win.
- **Compare against `std::unordered_map` alone:** micro-benchmark looking up a
  symbol once per message with ids vs. hashing strings every time.

## Files

- Stub: `src/symbol_table.cpp`
- Tests: `test/test_symbol_table.cpp`
- Reference: `SOLUTION.md`