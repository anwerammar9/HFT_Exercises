# Exercise 42 — Graph Search (BFS / DFS) (Task)

## The problem (in plain words)

Direct transfers are a graph, settlement latency is a hop count, and "can I
route through this chain of venues?" is reachability. Implement the two
canonical traversals from scratch on an **undirected** graph, then the three
things people actually search for: **hop distances** (BFS), **connected
components** (the venue clusters), and **cycle detection** (DFS back edges).
Traversal order must be **deterministic**, so adjacency is kept sorted.

## Requirements (what the tests check)

1. `add_edge(u, v)` — undirected (both sides), **duplicates collapse**, a
   self-loop counts as one edge; out-of-range vertices **throw
   `std::out_of_range`**.
2. `bfs(start)` — level-order visit; `dfs(start)` — **depth-first order
   matching a recursive first-visit search** (`0-1,0-2,1-3` from 0 →
   `{0,1,3,2}`); both return the visit order and deterministically.
3. `bfs_distance(start)` — hops to every vertex; unreachable → **-1**.
4. `connected_components()` — every component as one vector, the whole list
   deterministic.
5. `has_cycle()` — for an undirected graph, an undirected cycle (include
   self-loops); `add_edge` counts must stay consistent.

## Public API

```cpp
class Graph {
  explicit Graph(std::size_t vertices);
  std::size_t vertex_count() const noexcept;
  std::size_t edge_count() const noexcept;
  void add_edge(int u, int v);
  std::vector<int> bfs(int start) const;
  std::vector<int> dfs(int start) const;
  std::vector<int> bfs_distance(int start) const;
  std::vector<std::vector<int>> connected_components() const;
  bool has_cycle() const;
};
```

Stub in `src/graph_search.cpp`, class in `include/graph_search.h` (adjacency
lives in the class, so the stub .cpp is self-contained).

## How to think about it (suggested design)

- `adj_`: `std::vector<std::set<int>>` — sorted adjacency makes every visit
  order deterministic, and `std::set` makes duplicate edges collapse for free.
- **BFS:** a `std::queue`; mark a vertex seen **when it is enqueued** (else it
  can be enqueued twice).
- **DFS:** an explicit `std::stack` — push a node's neighbors in **reverse**
  so the *smallest* neighbor is popped and visited first, matching a recursive
  DFS's deepest-leftmost descent.
- **Cycle test:** DFS carrying `(node, parent)`; a neighbor that is already
  seen **and isn't the parent** is a back edge → cycle. With the depth-first
  stack there are no cross edges in undirected graphs, so this single test is
  enough.

## Make it harder (optional — not covered by the tests)

- **`shortest_path(start, goal)`:** reconstruct the actual path, not just the
  distance (record `parent[]` during BFS).
- **Directed graphs:** add an `add_directed`/`is_directed` mode and re-derive
  cycle detection (DFS colors: white/gray/black).
- **Topological order:** for the directed DAG case, produce a valid
  topological sort via DFS finish times or Kahn's algorithm.
- **Bipartite check / two-colorability** via BFS — the "can I split venues into
  two feed handler groups" test.

## Files

- Stub: `src/graph_search.cpp`
- Tests: `test/test_graph_search.cpp`
- Reference: `SOLUTION.md`
