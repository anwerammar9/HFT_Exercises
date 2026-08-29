#ifndef EXERCISE42_GRAPH_SEARCH_H_
#define EXERCISE42_GRAPH_SEARCH_H_

#include <cstddef>
#include <set>
#include <vector>

// BFS / DFS over an UNDIRECTED graph, adjacency stored in sorted
// `std::set`s so traversal order is deterministic (a requirement hidden in
// most book code: neighbor order is part of BFS/DFS order).
//
// Contract:
//   - vertices are 0..vertex_count()-1; add_edge beyond that range throws
//     std::out_of_range. Duplicate edges collapse (edge_count counts UNIQUE
//     undirected edges; a self-loop counts as one edge).
//   - bfs(start)/dfs(start): discovery (first-visit) order as a vector.
//     dfs uses an explicit stack and visits neighbors ascending, matching a
//     from-scratch recursive DFS first-visit order (deepest leftmost child
//     first). Both throw std::out_of_range for an invalid start.
//   - bfs_distance(start): shortest-path hop distance to every vertex in one
//     BFS; -1 for unreachable vertices.
//   - connected_components(): every component as a vector of its vertices,
//     components visited in ascending minimum-vertex order.
//   - has_cycle(): undirected cycle detection via DFS (a back edge to a
//     visited, non-parent vertex). Self-loops are cycles.
//
// TODO(anwer): implement the searches (see SOLUTION.md). The stubs return
// empty results / false / 0 edges so every traversal test runs RED without
// hanging or crashing.

class Graph {
 public:
  explicit Graph(std::size_t vertices);

  void add_edge(int u, int v);
  std::size_t vertex_count() const noexcept { return adj_.size(); }
  std::size_t edge_count() const noexcept { return edge_count_; }

  std::vector<int> bfs(int start) const;
  std::vector<int> dfs(int start) const;
  std::vector<int> bfs_distance(int start) const;
  std::vector<std::vector<int>> connected_components() const;
  bool has_cycle() const;

 private:
  void check_vertex(int v) const;  // throws std::out_of_range when invalid

  std::vector<std::set<int>> adj_;
  std::size_t edge_count_ = 0;
};

#endif  // EXERCISE42_GRAPH_SEARCH_H_