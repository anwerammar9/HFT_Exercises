# Exercise algorithms/04_binary_search_tree (ex41) — Binary Search Tree (Ordered Directory) (Task)

## The problem (in plain words)

A sorted price-level directory or incremental symbol index needs three things
a sorted array cannot give you cheaply: O(log n) insert, O(log n) erase, and
"—what's the closest active level to this stale quote?" Implement a
from-scratch **binary search tree** with the classic 0/1/2-child erase, and a
`nearest(key)` that works with a single directional walk.

## Requirements (what the tests check)

1. `insert(key)` — false if the key already exists (no duplicates, tree
   untouched); `erase(key)` — false if absent.
2. `contains(key)`, `size()`, `empty()`.
3. `min()`/`max()` — the extreme keys; **throw `std::out_of_range`** on an
   empty tree.
4. `nearest(key)` — the stored key minimizing `|key - stored|`; ties resolve to
   the **smaller** stored key (e.g. `|20-25| == |30-25|` → `20`); exact hits
   return immediately.
5. `in_order()` — a sorted `std::vector<int>` of all keys.
6. **All invariants survive heavy mutation**: inserting 0..99 in shuffled
   order, then erasing all evens, must leave exactly the odd keys — in order.
7. Value semantics: copy is **deep**, assignment replaces the contents, the
   destructor frees every node.

## Public API

```cpp
class BinarySearchTree {
  BinarySearchTree();  BinarySearchTree(const BinarySearchTree&);
  BinarySearchTree& operator=(const BinarySearchTree&);  ~BinarySearchTree();
  bool insert(int key);  bool erase(int key);
  bool contains(int key) const;
  int min() const;  int max() const;  int nearest(int key) const;
  std::size_t size() const noexcept;  bool empty() const noexcept;
  std::vector<int> in_order() const;
};
```

Stub in `src/binary_search_tree.cpp`, class in `include/binary_search_tree.h`.

## How to think about it (suggested design)

- Nodes: `struct Node { int key; std::unique_ptr<Node> left, right; };` and a
  `root_` + `size_` member.
- Recursive helpers take the slot **by reference** (`Node*&`), which makes
  splicing trivial:
  - **0 children:** drop the slot.
  - **1 child:** promote it.
  - **2 children:** overwrite the victim's key with its **in-order successor**
    (the leftmost node of the right subtree) and splice that successor out.
- `min`/`max`/`nearest` are pure descents; `in_order` is left/self/right.
- `nearest` doesn't enumerate: walk the branch toward `key`, tracking the best
  `|key - stored|` seen.

## Make it harder (optional — not covered by the tests)

- **Balance:** implement AVL rotations to guarantee O(log n) even for sorted
  inputs (the 0..99-in-order worst case).
- **Iterative ops:** rewrite insert/erase without recursion (RLL/RLR-pointer
  style) and compare stack behavior.
- **`rank_of(key)` / `kth(k)`:** stable ordering statistics on the tree — the
  basis of order book cumulative depth queries.
- **Range query:** `slice(lo, hi)` returning all keys in an interval in O(log n
  + k) — what a lightening sweep would ask the directory for.

## Files

- Stub: `src/binary_search_tree.cpp`
- Tests: `test/test_binary_search_tree.cpp`
- Reference: `SOLUTION.md`
