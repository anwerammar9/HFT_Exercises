# Exercise memory/10_refcount_gc (ex51) — Reference-Counting GC (Reference Solution)

**What you implement:** a CPython-style hybrid heap — prompt intrusive
reference counts for acyclic garbage plus `collect_cycles()` (internal-vs-
external analysis feeding a tracing mark) for the cyclic islands counting
cannot see.

**Approach**
- Nodes: `vector<unique_ptr<RcNode>>` registry + `unordered_set<RcNode*>`
  live set; `RcObj<T>` holds the value and forwards `trace()` only
  `if constexpr (RcTraceable<T>)`.
- `RcPtr` holds `(heap, node, ptr)` with the heap type-erased to `void*`;
  an `RcHeapAccess` bridge reaches the heap's private `destroy_now`/`unroot`
  (and the `sweeping_` flag) without exposing them. Copies bump, moves
  steal, drop-to-zero destroys + unregisters immediately — the cascade that
  frees chains for free. Self-assign is guarded.
- Roots pin without counting (`RcRooted`, move-only, `lock()` re-bumps).
- `collect_cycles()`: count internal refs (edges from heap nodes); seed with
  roots + nodes with `strong > internal` (refs from outside); mark; then
  sweep in **two phases** — detach the dead into a trash vector first,
  destroy after. The two phases matter: dead destructors drop refcounts,
  which must neither mutate the registry mid-loop nor destroy marked nodes,
  so drops are suppressed while `sweeping_` (liveness was already decided by
  the mark). Dead roots are pruned afterwards.
- Single-threaded by contract (plain counters; atomic counts were ex45).

## Reference API + implementation — `include/refcount_gc.h`

The classes are templates, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE51_REFCOUNT_GC_H_
#define EXERCISE51_REFCOUNT_GC_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename T>
class RcPtr;
template <typename T>
class RcRooted;

class RcNode;

// Mark worklist for the cycle collector.
struct RcTracer {
  explicit RcTracer(std::vector<RcNode*>* work) : work_(work) {}
  void visit(RcNode* n) {
    if (n != nullptr) work_->push_back(n);
  }

 private:
  std::vector<RcNode*>* work_;
};

// Outgoing edges are declared by the payload itself: define
//   void trace(RcTracer& tr) const;
// Payloads without trace() are leaves.
template <typename T>
concept RcTraceable = requires(const T& t, RcTracer& tr) { t.trace(tr); };

class RcNode {
 public:
  virtual ~RcNode() = default;
  virtual void trace(RcTracer& tr) const = 0;
  std::size_t strong = 0;  // owning RcPtr count (single-threaded)
  bool marked = false;
};

template <typename T>
class RcObj : public RcNode {
 public:
  template <typename... Args>
  explicit RcObj(Args&&... args) : value_(std::forward<Args>(args)...) {}
  void trace(RcTracer& tr) const override {
    if constexpr (RcTraceable<T>) value_.trace(tr);
  }
  T value_;
};

struct RcBump {};

template <typename T>
class RcPtr {
 public:
  RcPtr() noexcept : heap_(nullptr), node_(nullptr), ptr_(nullptr) {}
  RcPtr(const RcPtr& o) noexcept : heap_(o.heap_), node_(o.node_), ptr_(o.ptr_) {
    bump();
  }
  RcPtr(RcPtr&& o) noexcept : heap_(o.heap_), node_(o.node_), ptr_(o.ptr_) {
    o.heap_ = nullptr;
    o.node_ = nullptr;
    o.ptr_ = nullptr;
  }
  RcPtr& operator=(const RcPtr& o) noexcept {
    if (this != &o) {
      drop();
      heap_ = o.heap_;
      node_ = o.node_;
      ptr_ = o.ptr_;
      bump();
    }
    return *this;
  }
  RcPtr& operator=(RcPtr&& o) noexcept {
    if (this != &o) {
      drop();
      heap_ = o.heap_;
      node_ = o.node_;
      ptr_ = o.ptr_;
      o.heap_ = nullptr;
      o.node_ = nullptr;
      o.ptr_ = nullptr;
    }
    return *this;
  }
  ~RcPtr() { drop(); }

  void reset() noexcept {
    drop();
    heap_ = nullptr;
    node_ = nullptr;
    ptr_ = nullptr;
  }

  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(const RcPtr& o) const noexcept { return ptr_ == o.ptr_; }
  bool operator!=(const RcPtr& o) const noexcept { return ptr_ != o.ptr_; }

  std::size_t use_count() const noexcept {
    return node_ != nullptr ? node_->strong : 0;
  }

  // Public so payload trace() methods can name edges
  // (tr.visit(child.node())); not for general use.
  RcNode* node() const noexcept { return node_; }

 private:
  template <typename U>
  friend class RcRooted;
  friend class RcHeap;
  friend class RcHeapAccess;
  RcPtr(void* heap, RcNode* node, T* p, RcBump) noexcept
      : heap_(heap), node_(node), ptr_(p) {
    bump();
  }
  void bump() noexcept {
    if (node_ != nullptr) ++node_->strong;
  }
  void drop() noexcept;

  void* heap_ = nullptr;  // RcHeap*, type-erased to break the include cycle
  RcNode* node_ = nullptr;
  T* ptr_ = nullptr;
};

template <typename T>
class RcRooted {
 public:
  RcRooted() noexcept = default;
  RcRooted(const RcRooted&) = delete;
  RcRooted& operator=(const RcRooted&) = delete;
  RcRooted(RcRooted&& o) noexcept
      : heap_(o.heap_), node_(o.node_), ptr_(o.ptr_) {
    o.heap_ = nullptr;
    o.node_ = nullptr;
    o.ptr_ = nullptr;
  }
  RcRooted& operator=(RcRooted&& o) noexcept {
    if (this != &o) {
      release();
      heap_ = o.heap_;
      node_ = o.node_;
      ptr_ = o.ptr_;
      o.heap_ = nullptr;
      o.node_ = nullptr;
      o.ptr_ = nullptr;
    }
    return *this;
  }
  ~RcRooted() { release(); }

  // Pin + own: upgrade the root to a counted handle.
  RcPtr<T> lock() const;

 private:
  friend class RcHeap;
  RcRooted(void* heap, RcNode* node, T* p) noexcept
      : heap_(heap), node_(node), ptr_(p) {}
  void release() noexcept;

  void* heap_ = nullptr;
  RcNode* node_ = nullptr;
  T* ptr_ = nullptr;
};

// CPython-style hybrid heap: prompt reference counting for acyclic garbage
// (objects die the moment their last RcPtr drops — no collector needed) plus
// collect_cycles() for the cyclic garbage refcounting cannot see.
//
// Contract:
//   - make<T>(args...) creates an object with use_count() == 1.
//   - Copies bump, moves steal, drop-to-zero destroys IMMEDIATELY (and frees
//     the node): acyclic structures never need the collector.
//   - RcRooted<T> pins without counting (move-only RAII).
//   - collect_cycles() frees heap islands referenced only from inside the
//     heap (strong == internal) and unreachable from roots; returns count.
//     Rooted or externally-held cycles survive.
//   - Single-threaded by contract (plain counters, like ex46/47).
class RcHeap {
 public:
  RcHeap() = default;
  RcHeap(const RcHeap&) = delete;
  RcHeap& operator=(const RcHeap&) = delete;

  template <typename T, typename... Args>
  RcPtr<T> make(Args&&... args) {
    auto obj = std::make_unique<RcObj<T>>(std::forward<Args>(args)...);
    T* p = &obj->value_;
    RcNode* raw = obj.get();
    nodes_.push_back(std::move(obj));
    live_.insert(raw);
    return RcPtr<T>(this, raw, p, RcBump{});
  }

  template <typename T>
  RcRooted<T> root(const RcPtr<T>& h) {
    if (h.node() != nullptr) roots_.push_back(h.node());
    return RcRooted<T>(this, h.node(), h.get());
  }

  std::size_t collect_cycles() {
    sweeping_ = true;
    // Count internal refs: edges originating from other heap nodes.
    std::unordered_map<RcNode*, std::size_t> internal;
    std::vector<RcNode*> edges;
    for (auto& up : nodes_) {
      edges.clear();
      RcTracer tr(&edges);
      up->trace(tr);
      for (RcNode* m : edges) {
        if (m != nullptr && live_.find(m) != live_.end()) ++internal[m];
      }
    }
    // Seed: explicit roots + anything with refs from outside the heap.
    std::vector<RcNode*> stack = roots_;
    for (auto& up : nodes_) {
      RcNode* n = up.get();
      const std::size_t in = internal.count(n) != 0u ? internal[n] : 0u;
      if (n->strong > in) stack.push_back(n);
    }
    while (!stack.empty()) {
      RcNode* n = stack.back();
      stack.pop_back();
      if (n == nullptr || live_.find(n) == live_.end()) continue;
      if (n->marked) continue;
      n->marked = true;
      RcTracer tr(&stack);
      n->trace(tr);
    }
    // Sweep whatever was reachable only from inside (or from nothing).
    // Two phases: detach the dead from the registry FIRST, destroy them
    // after — their destructors drop refcounts, which must neither mutate
    // this registry mid-loop nor destroy live nodes (drops are suppressed
    // while sweeping_; liveness was already decided by the mark).
    std::vector<std::unique_ptr<RcNode>> trash;
    for (auto it = nodes_.begin(); it != nodes_.end();) {
      if (!(*it)->marked) {
        live_.erase(it->get());
        trash.push_back(std::move(*it));
        it = nodes_.erase(it);
      } else {
        (*it)->marked = false;
        ++it;
      }
    }
    const std::size_t freed = trash.size();
    trash.clear();  // runs payload dtors; refcount drops are inert here
    sweeping_ = false;
    // Drop roots that named freed nodes.
    std::size_t kept = 0;
    for (RcNode* r : roots_) {
      if (live_.find(r) != live_.end()) roots_[kept++] = r;
    }
    roots_.resize(kept);
    return freed;
  }

  template <typename T>
  bool is_live(const RcPtr<T>& h) const noexcept {
    return h.node() != nullptr && live_.find(h.node()) != live_.end();
  }

  std::size_t live() const noexcept { return live_.size(); }
  std::size_t roots() const noexcept { return roots_.size(); }

 private:
  friend class RcHeapAccess;
  void destroy_now(RcNode* n) {
    if (live_.erase(n) == 0u) return;  // already swept: nothing to detach
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
      if (it->get() == n) {
        nodes_.erase(it);
        return;
      }
    }
  }
  void unroot(RcNode* n) noexcept {
    for (std::size_t i = 0; i < roots_.size(); ++i) {
      if (roots_[i] == n) {
        roots_[i] = roots_.back();
        roots_.pop_back();
        return;
      }
    }
  }

  std::vector<std::unique_ptr<RcNode>> nodes_;
  std::unordered_set<RcNode*> live_;
  std::vector<RcNode*> roots_;
  bool sweeping_ = false;  // suppresses refcount-triggered destroy_now
};

// Access bridge: lets handles call private heap mechanics without exposing
// them publicly (defined after RcHeap is complete).
class RcHeapAccess {
 public:
  static void destroy_now(void* heap, RcNode* n) {
    static_cast<RcHeap*>(heap)->destroy_now(n);
  }
  static void unroot(void* heap, RcNode* n) {
    static_cast<RcHeap*>(heap)->unroot(n);
  }
  static bool is_sweeping(void* heap) {
    return static_cast<RcHeap*>(heap)->sweeping_;
  }
  template <typename T>
  static RcPtr<T> bumped(void* heap, RcNode* n, T* p) {
    return RcPtr<T>(heap, n, p, RcBump{});
  }
};

template <typename T>
void RcPtr<T>::drop() noexcept {
  if (node_ != nullptr) {
    // A zero count destroys immediately — unless the heap is mid-sweep, in
    // which case liveness was already decided by the mark and this drop only
    // retires a dead edge (destroying here could free a marked node or
    // mutate the registry under the collector).
    if (--node_->strong == 0 && !RcHeapAccess::is_sweeping(heap_))
      RcHeapAccess::destroy_now(heap_, node_);
    node_ = nullptr;
    ptr_ = nullptr;
    heap_ = nullptr;
  }
}

template <typename T>
void RcRooted<T>::release() noexcept {
  if (heap_ != nullptr && node_ != nullptr) {
    RcHeapAccess::unroot(heap_, node_);
    heap_ = nullptr;
    node_ = nullptr;
    ptr_ = nullptr;
  }
}

template <typename T>
RcPtr<T> RcRooted<T>::lock() const {
  if (heap_ == nullptr || node_ == nullptr) return RcPtr<T>();
  // The root pins the node, so it cannot have been swept: re-bump safely.
  return RcHeapAccess::bumped<T>(heap_, node_, ptr_);
}

#endif  // EXERCISE51_REFCOUNT_GC_H_
```
