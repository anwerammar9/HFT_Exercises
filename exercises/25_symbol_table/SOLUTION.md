# Exercise 25 — Symbol Table (Reference Solution)

**What you implement:** a thread-safe, two-way symbol ⇄ id registry. Feed
handlers intern ticker strings once so hot paths compare 4-byte ids instead of
hashing strings.

**Approach**
- Two structures: `symbol_to_id_` (unordered_map<string,int32_t>) and
  `id_to_symbol_` (vector<string> where index == id). Intern returns the
  existing id or appends `id = id_to_symbol_.size()`, pushes the string and a
  `live_` flag, and registers the forward mapping — ids are never reused.
- `lookup(id)` guards `id < 0 || id >= size` and the `live_` flag, returning a
  cheap `string_view` (retention: storage never freed until `clear()`, so
  late readers stay valid).
- `erase_symbol`/`erase_id` clear the forward mapping and mark `live_ = 0` so
  `lookup` reports nullopt, but the backing strings stay alive.
- `size()` counts `live_` flags (currently-registered, not slots), `clear()`
  drops everything and ids restart at 0.
- Thread safety: one `std::mutex` guards every method (const methods included);
  intern/lookup never dangle because of the retention policy.

## Reference API — `include/symbol_table.h`
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
## Reference implementation — `src/symbol_table.cpp`
#include "symbol_table.h"

#include <mutex>
#include <string_view>

std::int32_t SymbolTable::intern(std::string_view symbol) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = symbol_to_id_.find(std::string(symbol));
  if (it != symbol_to_id_.end()) return it->second;

  const std::int32_t id = static_cast<std::int32_t>(id_to_symbol_.size());
  id_to_symbol_.emplace_back(symbol);
  live_.push_back(1);
  symbol_to_id_.emplace(std::string(symbol), id);
  return id;
}

std::optional<std::int32_t> SymbolTable::find(std::string_view symbol) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = symbol_to_id_.find(std::string(symbol));
  if (it == symbol_to_id_.end()) return std::nullopt;
  return it->second;
}

std::optional<std::string_view> SymbolTable::lookup(std::int32_t id) const {
  std::lock_guard<std::mutex> lock(mu_);
  if (id < 0 || static_cast<std::size_t>(id) >= id_to_symbol_.size()) {
    return std::nullopt;
  }
  if (!live_[static_cast<std::size_t>(id)]) return std::nullopt;
  return std::string_view(id_to_symbol_[static_cast<std::size_t>(id)]);
}

bool SymbolTable::erase_symbol(std::string_view symbol) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = symbol_to_id_.find(std::string(symbol));
  if (it == symbol_to_id_.end()) return false;
  live_[static_cast<std::size_t>(it->second)] = 0;
  symbol_to_id_.erase(it);
  return true;
}

bool SymbolTable::erase_id(std::int32_t id) {
  std::lock_guard<std::mutex> lock(mu_);
  if (id < 0 || static_cast<std::size_t>(id) >= live_.size() ||
      !live_[static_cast<std::size_t>(id)]) {
    return false;
  }
  live_[static_cast<std::size_t>(id)] = 0;
  symbol_to_id_.erase(id_to_symbol_[static_cast<std::size_t>(id)]);
  return true;
}

std::size_t SymbolTable::size() const noexcept {
  std::lock_guard<std::mutex> lock(mu_);
  std::size_t n = 0;
  for (auto flag : live_) {
    if (flag) ++n;
  }
  return n;
}

void SymbolTable::clear() {
  std::lock_guard<std::mutex> lock(mu_);
  symbol_to_id_.clear();
  id_to_symbol_.clear();
  live_.clear();
}