#include "symbol_table.h"

#include <mutex>
#include <string_view>

// TODO(anwer): implement the two-way registry (see SOLUTION.md).
//
// Suggested shape:
//   intern():  lock; symbol_to_id_.find -> existing id, else assign
//              id = id_to_symbol_.size(), push string + live_ flag, map, return.
//   find():    lock; map lookup.
//   lookup():  lock; bounds-check id, must be live, return string_view into
//              the retained storage (never freed until clear()).
//   erase_*:   lock; unmap + clear live_ flag (storage retained).
//   size():    lock; count live_ flags.
//
// Stub: intern() always returns -1 (fails the sequential-id and concurrent
// tests RED without any out-of-bounds index — the API still bounds-checks on
// the empty tables).

std::int32_t SymbolTable::intern(std::string_view /*symbol*/) { return -1; }

std::optional<std::int32_t> SymbolTable::find(std::string_view /*symbol*/) const {
  return std::nullopt;
}

std::optional<std::string_view> SymbolTable::lookup(std::int32_t /*id*/) const {
  return std::nullopt;
}

bool SymbolTable::erase_symbol(std::string_view /*symbol*/) { return false; }

bool SymbolTable::erase_id(std::int32_t /*id*/) { return false; }

std::size_t SymbolTable::size() const noexcept { return 0; }

void SymbolTable::clear() {}