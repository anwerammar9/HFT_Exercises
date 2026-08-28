#ifndef EXERCISE22_LOCKFREE_STACK_H_
#define EXERCISE22_LOCKFREE_STACK_H_

#include <atomic>
#include <cstdint>
#include <new>
#include <utility>

// Lock-free LIFO stack (Treiber stack): singly-linked list claimed via CAS on
// the head pointer.
//
// Contract:
//   - Multiple concurrent producers + consumers.
//   - push(T): no failure mode (allocates a node like Treiber).
//   - try_pop(T&): false when empty; the out-param is untouched.
//
// ABA: nodes are NEVER reclaimed during operation — popped nodes park on an
// internal retired list (also never returned to the allocator) until the whole
// stack is destroyed. A node's fields are never written again after a pop, so
// a stale reader that races read-only on a popped node is safe (see SOLUTION.md
// for the full argument).
//
// TODO(anwer): implement push()/try_pop on head_ with a compare_exchange_weak
// retry loop, and the retire()-based reclamation. Stub below returns false from
// try_pop so the concurrent tests go RED without tearing the shared seen[].
template <typename T>
class LockFreeStack {
 public:
  LockFreeStack() = default;

  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  ~LockFreeStack() {
    Node* n = head_.load(std::memory_order_relaxed);
    while (n != nullptr) {
      Node* next = n->next;
      delete n;
      n = next;
    }
    RetiredCell* c = retired_.load(std::memory_order_relaxed);
    while (c != nullptr) {
      RetiredCell* next = c->next;
      delete c->node;
      delete c;
      c = next;
    }
  }

  void push(T /*value*/) {}  // TODO(anwer): fresh Node + CAS retry loop

  bool try_pop(T& /*out*/) { return false; }  // TODO(anwer): CAS head_, retire

 private:
  struct Node {
    T value;
    Node* next;
  };
  struct RetiredCell {
    Node* node;
    RetiredCell* next;
  };

  std::atomic<Node*> head_{nullptr};
  std::atomic<RetiredCell*> retired_{nullptr};
};

#endif  // EXERCISE22_LOCKFREE_STACK_H_