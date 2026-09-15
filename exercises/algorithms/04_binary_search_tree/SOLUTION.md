# Exercise algorithms/04_binary_search_tree (ex41) — Binary Search Tree (Ordered Directory) (Reference Solution)

**What you implement:** a from-scratch ordered BST over int keys — the structure
behind a sorted price-level directory or incremental symbol index where you need
"the closest active level to a target". Recursive insert / erase (0, 1 and
2-child cases), min/max by descent, `nearest` by one directional walk, and an
in-order traversal that must come out sorted.

**Approach**
- **insert_node(Node*& n, key):** recurse on the direction the key must live;
  a missed slot is allocated; duplicates return false and leave the tree
  untouched.
- **erase_node(Node*& n, key):** pass the slot by reference so splicing is
  trivial. 0 children → drop the slot; 1 child → promote it; 2 children →
  overwrite the victim's key with its in-order successor (the leftmost node of
  the right subtree) and splice that successor out. Start by LEFTIES-only traces
  prove erase returns false when the key is absent.
- **contains/min/max:** pure descents — left-most for min, right-most for max;
  both throw `std::out_of_range` when empty.
- **nearest(key):** one walk tracking the best `|key - stored|`; because the
  tree narrows the candidate region toward `key`, walking the branching
  direction is sufficient. Ties resolve to the SMALLER stored key
  (`|20-25| == |30-25|` → `20`), and an exact hit returns immediately.
- **in_order():** classic left/self/right recursion into a pre-reserved vector.
- **Value semantics:** copy deep-clones via recursion; assignment clone-then-
  destroy; the destructor post-order-deletes (sufficient for test depth —
  production code would amortize or thread the frees).

## Reference API — `include/binary_search_tree.h`
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
## Reference implementation — `src/binary_search_tree.cpp`
#include "binary_search_tree.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

BinarySearchTree::BinarySearchTree(const BinarySearchTree& other)
    : root_(clone(other.root_)), size_(other.size_) {}

BinarySearchTree& BinarySearchTree::operator=(const BinarySearchTree& other) {
  if (this != &other) {
    Node* fresh = clone(other.root_);
    destroy(root_);
    root_ = fresh;
    size_ = other.size_;
  }
  return *this;
}

BinarySearchTree::~BinarySearchTree() { destroy(root_); }

bool BinarySearchTree::insert(int key) {
  if (insert_node(root_, key)) {
    ++size_;
    return true;
  }
  return false;
}

bool BinarySearchTree::erase(int key) {
  if (erase_node(root_, key)) {
    --size_;
    return true;
  }
  return false;
}

bool BinarySearchTree::contains(int key) const {
  Node* n = root_;
  while (n) {
    if (key == n->key) return true;
    n = key < n->key ? n->left : n->right;
  }
  return false;
}

int BinarySearchTree::min() const {
  if (!root_) throw std::out_of_range("BinarySearchTree::min on empty");
  return find_min(root_)->key;
}

int BinarySearchTree::max() const {
  if (!root_) throw std::out_of_range("BinarySearchTree::max on empty");
  return find_max(root_)->key;
}

int BinarySearchTree::nearest(int key) const {
  if (!root_) throw std::out_of_range("BinarySearchTree::nearest on empty");
  const Node* cur = root_;
  int best = cur->key;
  while (cur) {
    const long long d = std::llabs(static_cast<long long>(cur->key) - key);
    const long long bd = std::llabs(static_cast<long long>(best) - key);
    if (d < bd || (d == bd && cur->key < best)) best = cur->key;
    if (key == cur->key) return key;
    cur = key < cur->key ? cur->left : cur->right;
  }
  return best;
}

std::vector<int> BinarySearchTree::in_order() const {
  std::vector<int> out;
  out.reserve(size_);
  collect_in_order(root_, out);
  return out;
}

// ---------------------------------------------------------------------------
// Recursive helpers
// ---------------------------------------------------------------------------

BinarySearchTree::Node* BinarySearchTree::clone(Node* n) {
  if (!n) return nullptr;
  Node* c = new Node(n->key);
  c->left = clone(n->left);
  c->right = clone(n->right);
  return c;
}

void BinarySearchTree::destroy(Node* n) {
  if (!n) return;
  destroy(n->left);
  destroy(n->right);
  delete n;
}

bool BinarySearchTree::insert_node(Node*& n, int key) {
  if (!n) {
    n = new Node(key);
    return true;
  }
  if (key == n->key) return false;
  return insert_node(key < n->key ? n->left : n->right, key);
}

bool BinarySearchTree::erase_node(Node*& n, int key) {
  if (!n) return false;
  if (key < n->key) return erase_node(n->left, key);
  if (key > n->key) return erase_node(n->right, key);

  Node* victim = n;
  if (!n->left && !n->right) {
    n = nullptr;
  } else if (!n->left) {
    n = n->right;
  } else if (!n->right) {
    n = n->left;
  } else {
    // two children: replace with the in-order successor (min of right)
    Node* parent = n;
    Node* succ = n->right;
    while (succ->left) {
      parent = succ;
      succ = succ->left;
    }
    n->key = succ->key;
    if (parent == n)
      n->right = succ->right;  // successor is the right child itself
    else
      parent->left = succ->right;
    victim = succ;
  }
  delete victim;
  return true;
}

const BinarySearchTree::Node* BinarySearchTree::find_min(const Node* n) {
  while (n->left) n = n->left;
  return n;
}

const BinarySearchTree::Node* BinarySearchTree::find_max(const Node* n) {
  while (n->right) n = n->right;
  return n;
}

void BinarySearchTree::collect_in_order(const Node* n, std::vector<int>& out) {
  if (!n) return;
  collect_in_order(n->left, out);
  out.push_back(n->key);
  collect_in_order(n->right, out);
}