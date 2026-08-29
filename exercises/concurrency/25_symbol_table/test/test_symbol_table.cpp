#include <gtest/gtest.h>

#include "symbol_table.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 8;
constexpr int kSymbolsPerThread = 1000;

std::string symbol_for(int t, int i) {
  return "SYM_" + std::to_string(t) + "_" + std::to_string(i);
}

// A bounded budget so a stub (which never registers anything) does not hang
// the stress test; a real, correct implementation finishes in milliseconds.
std::chrono::steady_clock::time_point deadline() {
  return std::chrono::steady_clock::now() + std::chrono::seconds(10);
}

}  // namespace

TEST(SymbolTableTest, InternsAssignSequentialIds) {
  SymbolTable st;
  EXPECT_EQ(st.intern("AAPL"), 0);
  EXPECT_EQ(st.intern("MSFT"), 1);
  EXPECT_EQ(st.intern("GOOG"), 2);
  EXPECT_EQ(st.size(), 3u);
}

TEST(SymbolTableTest, InternIsIdempotent) {
  SymbolTable st;
  int first = st.intern("AAPL");
  ASSERT_EQ(first, 0);
  EXPECT_EQ(st.intern("AAPL"), first);
  EXPECT_EQ(st.size(), 1u);
}

TEST(SymbolTableTest, LookupBothDirections) {
  SymbolTable st;
  ASSERT_EQ(st.intern("ZOOM"), 0);
  EXPECT_EQ(st.find("ZOOM").value_or(-1), 0);
  ASSERT_TRUE(st.lookup(0).has_value());
  EXPECT_EQ(st.lookup(0).value(), "ZOOM");
  EXPECT_FALSE(st.find("NOPE").has_value());
  EXPECT_FALSE(st.lookup(5).has_value());
}

TEST(SymbolTableTest, CaseSensitive) {
  SymbolTable st;
  ASSERT_EQ(st.intern("AAPL"), 0);
  EXPECT_FALSE(st.find("aapl").has_value());
  EXPECT_EQ(st.intern("aapl"), 1);  // distinct symbol
  EXPECT_EQ(st.size(), 2u);
}

TEST(SymbolTableTest, EraseBreaksLookupsAndNeverReusesIds) {
  SymbolTable st;
  ASSERT_EQ(st.intern("MSFT"), 0);
  ASSERT_EQ(st.intern("IBM"), 1);

  ASSERT_TRUE(st.erase_symbol("MSFT"));
  EXPECT_EQ(st.size(), 1u);
  EXPECT_FALSE(st.find("MSFT").has_value());
  EXPECT_FALSE(st.lookup(0).has_value());
  EXPECT_TRUE(st.find("IBM").has_value());
  EXPECT_FALSE(st.erase_symbol("MSFT"));  // already gone

  // Ids never reused: re-intern takes a FRESH id.
  EXPECT_EQ(st.intern("MSFT"), 2);
  EXPECT_EQ(st.size(), 2u);
  EXPECT_TRUE(st.lookup(2).has_value());

  // Erase by id too.
  ASSERT_TRUE(st.erase_id(1));
  EXPECT_FALSE(st.lookup(1).has_value());
  EXPECT_EQ(st.size(), 1u);
}

TEST(SymbolTableTest, ClearRestsAndRestartsIds) {
  SymbolTable st;
  st.intern("A");
  st.intern("B");
  ASSERT_EQ(st.size(), 2u);

  st.clear();
  EXPECT_EQ(st.size(), 0u);
  EXPECT_FALSE(st.find("A").has_value());
  EXPECT_FALSE(st.lookup(0).has_value());
  EXPECT_EQ(st.intern("A"), 0);  // ids restart from zero
  EXPECT_EQ(st.size(), 1u);
}

TEST(SymbolTableTest, ConcurrentInternsAreUniqueAndResolvable) {
  SymbolTable st;
  std::vector<std::vector<std::pair<std::string, int32_t>>> results(kThreads);

  std::vector<std::thread> workers;
  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&, t] {
      auto dl = deadline();
      for (int i = 0; i < kSymbolsPerThread && std::chrono::steady_clock::now() < dl; ++i) {
        std::string sym = symbol_for(t, i);
        int32_t id = st.intern(sym);
        results[t].emplace_back(sym, id);
      }
    });
  }
  for (auto& w : workers) w.join();

  // Every worker got an id for every symbol it tried.
  std::vector<std::pair<std::string, int32_t>> all;
  for (auto& r : results) {
    ASSERT_EQ(r.size(), static_cast<std::size_t>(kSymbolsPerThread));
    all.insert(all.end(), r.begin(), r.end());
  }
  ASSERT_EQ(all.size(), static_cast<std::size_t>(kThreads) * kSymbolsPerThread);

  // Sequential, unique, non-negative ids.
  std::set<int32_t> ids;
  for (auto& [sym, id] : all) {
    EXPECT_GE(id, 0);
    EXPECT_TRUE(ids.insert(id).second) << "duplicate id " << id;
  }
  EXPECT_EQ(ids.size(), all.size());

  // Every interned symbol resolves both ways.
  for (auto& [sym, id] : all) {
    EXPECT_EQ(st.find(sym).value_or(-1), id) << sym;
    ASSERT_TRUE(st.lookup(id).has_value()) << "id " << id;
    EXPECT_EQ(st.lookup(id).value(), sym);
  }
  EXPECT_EQ(st.size(), all.size());
}