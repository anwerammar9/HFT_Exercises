# Exercise concurrency/06_lockfree_stack (ex22) — Lock-Free Stack (Treiber) (Reference Solution)

**What you implement:** a Treiber stack — a single linked list claimed by CAS on
the head — with the ABA hazard solved by *memory retention* (nodes are never
reclaimed during the stack's lifetime).

**Approach**
- `push(T)`: allocate a fresh `Node{value, head}`; CAS `head_` from the loaded
  head to the new node; on failure the CAS reloads `head_` and we retry (the
  node->next write happens before the release-CAS so the pointers stay acyclic).
- `try_pop(T&)`: load `head_` acquire; `nullptr` ⇒ empty (leave `out`
  untouched). Read `next`, CAS `head_ next→next`; on success move the value out
  and `retire(cur)`.
- **ABA + reclamation**: popped nodes are parked on an internal `retired_`
  single-linked list via a `RetiredCell` *wrapper* — the node's own fields
  (notably `next`) are NEVER written again after a pop, so a stale reader is
  safe (read-only racers are fine). Because no address is ever reused while the
  stack lives, the classic ABA "same address, different node" failure cannot
  occur. The memory-retention tradeoff is documented; a production design would
  use hazard pointers or epoch reclamation.
- Destructor: walk and free the live list, then the retired list.

## Reference API — `include/lockfree_stack.h`
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
//   - push(T): no failure mode (always succeeds, allocates a node like Treiber).
//   - try_pop(T&): false when the stack is empty; the out-param is untouched.
//
// ABA handling: nodes are NOT reclaimed during operation. push() allocates a
// fresh node and popped nodes are parked on an internal retired list (also
// never returned to the allocator) until the whole stack is destroyed, at
// which point both the live list and the retired list are freed. A node's
// fields are never written again after it is popped, so a stale reader that
// races read-only on a popped node's `next` is safe: no addresses are ever
// reused while the stack lives, which makes the classic Treiber ABA failure
// (a stale head that has been popped, freed, re-pushed at the same address)
// impossible by construction. try_pop() may therefore safely dereference the
// current head's `next` field before the CAS without a use-after-free hazard.
// This memory-retention tradeoff is documented per the exercise; a production
// version would substitute hazard pointers or epoch reclamation.

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

  void push(T value) {
    Node* node = new Node{std::move(value), nullptr};
    Node* cur = head_.load(std::memory_order_relaxed);
    for (;;) {
      node->next = cur;
      if (head_.compare_exchange_weak(cur, node, std::memory_order_release,
                                      std::memory_order_relaxed)) {
        return;
      }
      // cur reloaded by compare_exchange_weak on failure; retry.
    }
  }

  bool try_pop(T& out) {
    Node* cur = head_.load(std::memory_order_acquire);
    for (;;) {
      if (cur == nullptr) return false;  // empty; out untouched
      // Safe to chase next before claiming: node addresses are never reused.
      Node* next = cur->next;
      if (head_.compare_exchange_weak(cur, next, std::memory_order_acq_rel,
                                      std::memory_order_acquire)) {
        out = std::move(cur->value);
        retire(cur);  // retained (never freed) until destruction; see ABA note
        return true;
      }
      // cur reloaded by compare_exchange_weak on failure; retry.
    }
  }

 private:
  struct Node {
    T value;
    Node* next;
  };
  // A popped node is parked in a wrapper so the node's OWN fields (notably
  // `next`) are never written again while it is still observable by a stale
  // try_pop reader. The wrapper chain is touched only by retire() and the
  // post-join destructor.
  struct RetiredCell {
    Node* node;
    RetiredCell* next;
  };

  void retire(Node* n) {
    RetiredCell* cell = new RetiredCell{n, nullptr};
    RetiredCell* r = retired_.load(std::memory_order_relaxed);
    for (;;) {
      cell->next = r;
      if (retired_.compare_exchange_weak(r, cell, std::memory_order_release,
                                         std::memory_order_relaxed)) {
        return;
      }
    }
  }

  std::atomic<Node*> head_{nullptr};
  std::atomic<RetiredCell*> retired_{nullptr};
};

#endif  // EXERCISE22_LOCKFREE_STACK_H_
## Reference TU — `src/lockfree_stack.cpp`
#include "lockfree_stack.h"

#include <cstdint>
#include <string>

// Implementation is inline in the header (see the TODO(anwer) markers).
// Keeping this TU pins the instantiations exercised by the unit tests.

template class LockFreeStack<int>;
template class LockFreeStack<std::uint64_t>;
template class LockFreeStack<std::string>;