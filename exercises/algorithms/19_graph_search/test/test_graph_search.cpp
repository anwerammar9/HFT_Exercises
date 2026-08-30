#include <gtest/gtest.h>

#include "graph_search.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {

// Build path graph: (0-1), (1-2), ..., (n-2)-(n-1)
Graph PathGraph(int n) {
  Graph g(n);
  for (int i = 0; i + 1 < n; ++i) g.add_edge(i, i + 1);
  return g;
}

}  // namespace

TEST(GraphSearchTest, VertexAndEdgeBookkeeping) {
  Graph g(5);
  EXPECT_EQ(g.vertex_count(), 5u);
  EXPECT_EQ(g.edge_count(), 0u);
  g.add_edge(0, 1);
  g.add_edge(0, 1);  // duplicate collapses
  g.add_edge(1, 0);  // reverse duplicate collapses too
  g.add_edge(2, 2);  // self-loop = one edge
  EXPECT_EQ(g.edge_count(), 2u);
  EXPECT_THROW(g.add_edge(5, 0), std::out_of_range);
  EXPECT_THROW(g.add_edge(0, -1), std::out_of_range);
}

TEST(GraphSearchTest, BfsOrderOnPathAndStar) {
  EXPECT_EQ(PathGraph(4).bfs(0), (std::vector<int>{0, 1, 2, 3}));

  Graph star(4);
  for (int v = 1; v < 4; ++v) star.add_edge(0, v);
  EXPECT_EQ(star.bfs(0), (std::vector<int>{0, 1, 2, 3}));

  EXPECT_EQ(PathGraph(4).bfs(2), (std::vector<int>{2, 1, 3, 0}));
}

TEST(GraphSearchTest, DfsOrderIsDepthFirstAndDeterministic) {
  // shape: 0-1, 0-2, 1-3  -> DFS from 0 goes deepest-left first
  Graph g(4);
  g.add_edge(0, 1);
  g.add_edge(0, 2);
  g.add_edge(1, 3);
  EXPECT_EQ(g.dfs(0), (std::vector<int>{0, 1, 3, 2}));

  EXPECT_EQ(PathGraph(6).dfs(0), (std::vector<int>{0, 1, 2, 3, 4, 5}));

  Graph star(4);
  for (int v = 1; v < 4; ++v) star.add_edge(0, v);
  EXPECT_EQ(star.dfs(0), (std::vector<int>{0, 1, 2, 3}));
}

TEST(GraphSearchTest, InvalidStartThrows) {
  Graph g(3);
  EXPECT_THROW(g.bfs(3), std::out_of_range);
  EXPECT_THROW(g.dfs(-1), std::out_of_range);
  EXPECT_THROW(g.bfs_distance(9), std::out_of_range);
}

TEST(GraphSearchTest, BfsDistanceShortestHops) {
  Graph g(6);
  // 0-1-2-3 and 0-4-5
  g.add_edge(0, 1);
  g.add_edge(1, 2);
  g.add_edge(2, 3);
  g.add_edge(0, 4);
  g.add_edge(4, 5);
  const auto dist = g.bfs_distance(0);
  EXPECT_EQ(dist, (std::vector<int>{0, 1, 2, 3, 1, 2}));

  // unreachable
  Graph disconnected(4);
  disconnected.add_edge(0, 1);
  disconnected.add_edge(2, 3);
  const auto d2 = disconnected.bfs_distance(0);
  ASSERT_EQ(d2.size(), 4u);
  EXPECT_EQ(d2[0], 0);
  EXPECT_EQ(d2[1], 1);
  EXPECT_EQ(d2[2], -1);
  EXPECT_EQ(d2[3], -1);
}

TEST(GraphSearchTest, ConnectedComponents) {
  Graph g(5);
  g.add_edge(0, 1);
  g.add_edge(2, 3);
  // 4 isolated
  const auto comps = g.connected_components();
  ASSERT_EQ(comps.size(), 3u);
  EXPECT_EQ(comps[0], (std::vector<int>{0, 1}));
  EXPECT_EQ(comps[1], (std::vector<int>{2, 3}));
  EXPECT_EQ(comps[2], (std::vector<int>{4}));
}

TEST(GraphSearchTest, CycleDetection) {
  // tree: no cycle
  Graph tree(4);
  tree.add_edge(0, 1);
  tree.add_edge(0, 2);
  tree.add_edge(1, 3);
  EXPECT_FALSE(tree.has_cycle());

  // triangle: cycle
  Graph tri(3);
  tri.add_edge(0, 1);
  tri.add_edge(1, 2);
  tri.add_edge(2, 0);
  EXPECT_TRUE(tri.has_cycle());

  // self-loop is a cycle
  Graph loop(2);
  loop.add_edge(0, 0);
  EXPECT_TRUE(loop.has_cycle());

  // empty graph / single vertex: no cycle
  Graph empty(0);
  EXPECT_FALSE(empty.has_cycle());
  Graph one(1);
  EXPECT_FALSE(one.has_cycle());
}

TEST(GraphSearchTest, DeterministicRandomGraph) {
  Graph g(10);
  for (int v = 1; v < 10; ++v) g.add_edge(0, v);
  for (int v = 1; v < 10; ++v) g.add_edge(v, (v + 1) % 10);  // one extra loop
  const auto b1 = g.bfs(0);
  const auto b2 = g.bfs(0);
  const auto d1 = g.dfs(0);
  const auto d2 = g.dfs(0);
  EXPECT_EQ(b1, b2);
  EXPECT_EQ(d1, d2);
  EXPECT_EQ(b1.size(), g.vertex_count());
  EXPECT_EQ(d1.size(), g.vertex_count());

  auto comps = g.connected_components();
  ASSERT_EQ(comps.size(), 1u);
  EXPECT_EQ(comps[0].size(), 10u);
  EXPECT_TRUE(g.has_cycle());  // the extra loop edges close a cycle
}