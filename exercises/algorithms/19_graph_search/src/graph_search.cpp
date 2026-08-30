#include "graph_search.h"

// TODO(anwer): implement the searches (see SOLUTION.md).
//
// Suggested shape:
//   - adjacency in sorted sets so BFS/DFS order is deterministic; add_edge
//     inserts on both sides and bumps edge_count_ only for genuinely new
//     edges (self-loops count once).
//   - bfs: queue + seen-on-enqueue; dfs: explicit stack pushing neighbors in
//     REVERSE to reproduce recursive first-visit order.
//   - bfs_distance: BFS stamping hops into a dist[] initialised to -1.
//   - connected_components: BFS every unvisited vertex.
//   - has_cycle: DFS carrying (node, parent); a seen, non-parent neighbor is
//     a back edge.
//
// Stub: no edges/traversals — every search is empty and has_cycle is false,
// so the traversal tests run RED without hanging or crashing.

Graph::Graph(std::size_t vertices) : adj_(vertices) {}

void Graph::add_edge(int /*u*/, int /*v*/) {}

std::vector<int> Graph::bfs(int /*start*/) const { return {}; }
std::vector<int> Graph::dfs(int /*start*/) const { return {}; }
std::vector<int> Graph::bfs_distance(int /*start*/) const { return {}; }
std::vector<std::vector<int>> Graph::connected_components() const { return {}; }
bool Graph::has_cycle() const { return false; }