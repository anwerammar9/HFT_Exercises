#ifndef EXERCISE25_SYMBOL_TABLE_H_
#define EXERCISE25_SYMBOL_TABLE_H_

#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Thread-safe, two-way, low-latency symbol <-> id registry.
// This is the "symbol interning" table a feed handler keeps so hot paths can
// compare 4-byte ids instead of hashing strings.
//
// Contract:
//   - intern(symbol): returns the existing id, or assigns the next
//     monotonic id (0, 1, 2, ...) and registers the string. Ids are NEVER
//     reused, even after erase (holes are fine in a trading day).
//     Case-sensitive; exact string match.
//   - find(symbol): id or nullopt.
//   - lookup(id): the interning string_view or nullopt. A string_view is
//     returned (no allocation) and stays valid until clear() — interned
//     strings are never freed before then (low-latency memory-retention).
//   - erase_sym/erase_id: remove one direction; false when absent. The
//     storage backing earlier string_views also stays alive (retention),
//     but lookup() must then report nullopt.
//   - size(): number of *currently registered* symbols (not slots used).
//   - clear(): drop everything; ids restart from 0 afterwards.
//
// Thread safety: all public methods are internally synchronized (the exercise
// is about the two-way structure + id bookkeeping beyond a plain
// std::unordered_map — e.g. string_views that must not dangle, and erase
// discipline). A mutex is provided; decide whether a lock-free read path
// (read-copy-update via atomic shared_ptr) is worth it.

class SymbolTable {
 public:
  std::int32_t intern(std::string_view symbol);
  std::optional<std::int32_t> find(std::string_view symbol) const;
  std::optional<std::string_view> lookup(std::int32_t id) const;
  bool erase_symbol(std::string_view symbol);
  bool erase_id(std::int32_t id);
  std::size_t size() const noexcept;
  void clear();

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, std::int32_t> symbol_to_id_;
  std::vector<std::string> id_to_symbol_;  // index == id; retained on erase
  std::vector<std::uint8_t> live_;         // index == id; 1 while registered
};

#endif  // EXERCISE25_SYMBOL_TABLE_H_