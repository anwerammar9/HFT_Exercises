#ifndef EXERCISE41_BINARY_SEARCH_TREE_H_
#define EXERCISE41_BINARY_SEARCH_TREE_H_

#include <cstddef>
#include <vector>

// From-scratch ordered binary search tree (int keys) — the structure behind a
// sorted price-level directory or an ordered symbol index where you need "the
// closest active level to a target".
//
// Contract:
//   - insert(k): true iff k was NOT already present (duplicates are rejected).
//   - erase(k): true iff k was present; restores BST shape for 0/1/2-child
//     nodes (a 2-child node is replaced by its in-order successor).
//   - contains(k), size(), empty().
//   - min()/max(): smallest/largest key; throw std::out_of_range when empty.
//   - nearest(k): the stored key closest to k by |k - stored|; a tie picks the
//     SMALLER key (e.g. {20,30}, nearest(25) == 20); throws when empty.
//   - in_order(): all keys ascending (a from-scratch in-order traversal).
//
// TODO(anwer): implement the recursive node ops (see SOLUTION.md). The stubs
// hold no nodes, return defaults/0-size, and throw std::logic_error on the
// non-const mutators so every test runs RED without hanging or crashing.

class BinarySearchTree {
 public:
  BinarySearchTree() = default;
  BinarySearchTree(const BinarySearchTree& other);             // deep copy
  BinarySearchTree& operator=(const BinarySearchTree& other);
  ~BinarySearchTree();

  bool insert(int key);
  bool erase(int key);
  bool contains(int key) const;
  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  int min() const;
  int max() const;
  int nearest(int key) const;
  std::vector<int> in_order() const;

 private:
  struct Node {
    int key;
    Node* left;
    Node* right;
    explicit Node(int k) : key(k), left(nullptr), right(nullptr) {}
  };

  static Node* clone(Node* n);
  static void destroy(Node* n);
  static bool insert_node(Node*& n, int key);
  static bool erase_node(Node*& n, int key);
  static const Node* find_min(const Node* n);
  static const Node* find_max(const Node* n);
  static void collect_in_order(const Node* n, std::vector<int>& out);

  Node* root_ = nullptr;
  std::size_t size_ = 0;
};

#endif  // EXERCISE41_BINARY_SEARCH_TREE_H_