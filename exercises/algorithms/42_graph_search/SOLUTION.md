# Exercise 42 — Graph Search (BFS / DFS) (Reference Solution)

**What you implement:** the two canonical traversals over an UNDIRECTED graph —
BFS (queue) and DFS (explicit stack, matching recursive first-visit order) —
plus the three things built on them: single-source hop distances (BFS), the
connected-component decomposition, and undirected cycle detection (DFS back
edges). Adjacency is kept in sorted `std::set`s so traversal order is
deterministic end to end.

**Approach**
- **Data model:** `std::vector<std::set<int>> adj_`; `add_edge` inserts on both
  sides and bumps `edge_count_` only when a genuinely new edge appears
  (duplicates collapse; a self-loop counts once). Out-of-range vertices throw
  `std::out_of_range`.
- **bfs(start):** a `std::queue` with a seen-flag set *at enqueue* (so a vertex
  is enqueued once); visiting the sorted adjacency yields the classic level
  order: `PathGraph(4).bfs(0)` = `{0,1,2,3}`.
- **dfs(start):** an explicit LIFO stack; the key determinism trick is pushing
  neighbors in REVERSE (`.rbegin()`), so the smallest neighbor is popped and
  visited first — reproducing a recursive DFS's deepest-leftmost descent
  (`0-1,0-2,1-3` → `{0,1,3,2}`).
- **bfs_distance(start):** the same BFS, but `dist` (init −1) is stamped when a
  vertex is discovered; unreachable vertices stay −1.
- **connected_components():** BFS every unvisited vertex; each BFS run is one
  component, discovered in ascending min-vertex order.
- **has_cycle():** DFS carrying `(node, parent)`. A neighbor that is already
  seen AND is not the parent is a back edge to an ancestor → cycle. Because the
  stack reproduces a depth-first descent, undirected DFS graphs have no cross
  edges, so that single test is sound (self-loops included).

## Reference API — `include/graph_search.h`
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
## Reference implementation — `src/graph_search.cpp`
#include "graph_search.h"

#include <queue>
#include <stack>
#include <stdexcept>
#include <utility>

Graph::Graph(std::size_t vertices) : adj_(vertices) {}

void Graph::check_vertex(int v) const {
  if (v < 0 || static_cast<std::size_t>(v) >= adj_.size())
    throw std::out_of_range("Graph: vertex out of range");
}

void Graph::add_edge(int u, int v) {
  check_vertex(u);
  check_vertex(v);
  if (u == v) {
    if (adj_[u].insert(v).second) ++edge_count_;
    return;
  }
  if (adj_[u].insert(v).second) {
    adj_[v].insert(u);
    ++edge_count_;
  }
}

std::vector<int> Graph::bfs(int start) const {
  check_vertex(start);
  const std::size_t n = adj_.size();
  std::vector<int> order;
  order.reserve(n);
  std::vector<char> seen(n, 0);
  std::queue<int> q;
  seen[start] = 1;
  q.push(start);
  while (!q.empty()) {
    const int u = q.front();
    q.pop();
    order.push_back(u);
    for (const int v : adj_[u]) {
      if (!seen[v]) {
        seen[v] = 1;
        q.push(v);
      }
    }
  }
  return order;
}

std::vector<int> Graph::dfs(int start) const {
  check_vertex(start);
  const std::size_t n = adj_.size();
  std::vector<int> order;
  order.reserve(n);
  std::vector<char> seen(n, 0);
  std::stack<int> st;
  st.push(start);
  while (!st.empty()) {
    const int u = st.top();
    st.pop();
    if (seen[u]) continue;
    seen[u] = 1;
    order.push_back(u);
    // push neighbors in REVERSE so the smallest is popped (and visited) first
    for (auto it = adj_[u].rbegin(); it != adj_[u].rend(); ++it) {
      if (!seen[*it]) st.push(*it);
    }
  }
  return order;
}

std::vector<int> Graph::bfs_distance(int start) const {
  check_vertex(start);
  const std::size_t n = adj_.size();
  std::vector<int> dist(n, -1);
  std::queue<int> q;
  dist[start] = 0;
  q.push(start);
  while (!q.empty()) {
    const int u = q.front();
    q.pop();
    for (const int v : adj_[u]) {
      if (dist[v] == -1) {
        dist[v] = dist[u] + 1;
        q.push(v);
      }
    }
  }
  return dist;
}

std::vector<std::vector<int>> Graph::connected_components() const {
  const std::size_t n = adj_.size();
  std::vector<char> seen(n, 0);
  std::vector<std::vector<int>> comps;
  for (std::size_t s = 0; s < n; ++s) {
    if (seen[s]) continue;
    std::vector<int> comp;
    std::queue<int> q;
    seen[s] = 1;
    q.push(static_cast<int>(s));
    while (!q.empty()) {
      const int u = q.front();
      q.pop();
      comp.push_back(u);
      for (const int v : adj_[u]) {
        if (!seen[v]) {
          seen[v] = 1;
          q.push(v);
        }
      }
    }
    comps.push_back(std::move(comp));
  }
  return comps;
}

bool Graph::has_cycle() const {
  const std::size_t n = adj_.size();
  std::vector<char> seen(n, 0);
  for (std::size_t s = 0; s < n; ++s) {
    if (seen[s]) continue;
    std::stack<std::pair<int, int>> st;  // (node, parent)
    st.push({static_cast<int>(s), -1});
    seen[s] = 1;
    while (!st.empty()) {
      const auto [u, parent] = st.top();
      st.pop();
      for (const int v : adj_[u]) {
        if (!seen[v]) {
          seen[v] = 1;
          st.push({v, u});
        } else if (v != parent) {
          return true;  // back edge to a visited, non-parent vertex
        }
      }
    }
  }
  return false;
}