#include "binary_search_tree.h"

// TODO(anwer): implement the recursive node ops (see SOLUTION.md).
//
// Suggested shape:
//   - insert_node(Node*& n, key): recurse by direction; false on a duplicate.
//   - erase_node(Node*& n, key): 0/1/2-child cases; a 2-child node is
//     replaced with its in-order successor (leftmost of the right subtree).
//   - min/max: descend left/right; throw std::out_of_range when empty.
//   - nearest(key): one walk tracking |key - stored|; ties -> smaller key.
//   - in_order(): left/self/right recursion into a pre-reserved vector.
//
// Stub: an empty tree — inserts/erases/contains answer false, accessors
// return 0, every traversal is empty, so the tests run RED without crashing.

BinarySearchTree::BinarySearchTree(const BinarySearchTree&) {}

BinarySearchTree& BinarySearchTree::operator=(const BinarySearchTree&) {
  return *this;
}

BinarySearchTree::~BinarySearchTree() {}

bool BinarySearchTree::insert(int) { return false; }
bool BinarySearchTree::erase(int) { return false; }
bool BinarySearchTree::contains(int) const { return false; }

int BinarySearchTree::min() const { return 0; }
int BinarySearchTree::max() const { return 0; }
int BinarySearchTree::nearest(int) const { return 0; }
std::vector<int> BinarySearchTree::in_order() const { return {}; }