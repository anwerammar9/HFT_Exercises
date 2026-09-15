# Exercise memory/09_mark_sweep (ex50) — Mark-Sweep Collector (Reference Solution)

**What you implement:** the classic tracing collector — stable slots, roots,
`trace()` edges, an iterative mark worklist, and an exact sweep that reclaims
*anywhere* in the heap. No reference counts: cycles die like everything else.

**Approach**
- Slots: `std::list<unique_ptr<GcSlot>>` (node addresses stable across
  sweeps) plus an `unordered_set<GcSlot*>` live set, so `is_live()` never
  dereferences freed memory. Roots: `vector<GcSlot*>`.
- `GcSlotFor<T>` holds the object and forwards `trace()` only
  `if constexpr (GcTraceable<T>)` (leaves need no method).
- `collect()`: seed the worklist with roots; mark iteratively, skipping null,
  already-swept (not in the live set), and already-marked slots; then erase
  every unmarked slot from the list *and* the live set and unmark survivors.
  Any root entry still naming a slot afterwards is live by construction —
  freed slots never had a live root (else they would have been marked).
- `GcRooted`'s dtor/move removes exactly one root registration. Handles are
  non-owning `(slot, ptr)` pairs; `slot_ptr()` is public so payloads can name
  edges. Single-threaded by contract.

## Reference API + implementation — `include/mark_sweep.h`

The classes are templates, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE50_MARK_SWEEP_H_
#define EXERCISE50_MARK_SWEEP_H_

#include <cstddef>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename T>
class GcPtr;
template <typename T>
class GcRooted;

class GcSlot;

// Mark worklist: payloads name their outgoing edges via visit(). Slots live
// in a std::list, so their addresses are stable across sweeps.
struct GcTracer {
  explicit GcTracer(std::vector<GcSlot*>* work) : work_(work) {}
  void visit(GcSlot* s) {
    if (s != nullptr) work_->push_back(s);
  }

 private:
  std::vector<GcSlot*>* work_;
};

// Outgoing edges are declared by the payload itself: define
//   void trace(GcTracer& tr) const;
// Payloads without trace() are leaves.
template <typename T>
concept GcTraceable = requires(const T& t, GcTracer& tr) { t.trace(tr); };

class GcSlot {
 public:
  virtual ~GcSlot() = default;
  virtual void trace(GcTracer& tr) const = 0;
  bool marked = false;
};

template <typename T>
class GcSlotFor : public GcSlot {
 public:
  template <typename... Args>
  explicit GcSlotFor(Args&&... args) : obj_(std::forward<Args>(args)...) {}
  void trace(GcTracer& tr) const override {
    if constexpr (GcTraceable<T>) obj_.trace(tr);
  }
  T obj_;
};

// Non-owning handle to a heap object. Copyable, nullable. The object may be
// reclaimed by collect() unless reachable from a root — check is_live()
// before touching a handle across a collection.
template <typename T>
class GcPtr {
 public:
  GcPtr() noexcept : slot_(nullptr), ptr_(nullptr) {}
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(const GcPtr& o) const noexcept { return ptr_ == o.ptr_; }

  // Public so payload trace() methods can name edges
  // (tr.visit(child.slot_ptr())); not for general use.
  GcSlot* slot_ptr() const noexcept { return slot_; }

 private:
  friend class GcHeap;
  template <typename U>
  friend class GcRooted;
  GcPtr(GcSlot* slot, T* p) noexcept : slot_(slot), ptr_(p) {}

  GcSlot* slot_;
  T* ptr_;
};

class GcHeap;

template <typename T>
class GcRooted {
 public:
  GcRooted() noexcept = default;
  GcRooted(const GcRooted&) = delete;
  GcRooted& operator=(const GcRooted&) = delete;
  GcRooted(GcRooted&& o) noexcept
      : heap_(o.heap_), slot_(o.slot_), ptr_(o.ptr_) {
    o.heap_ = nullptr;
    o.slot_ = nullptr;
    o.ptr_ = nullptr;
  }
  GcRooted& operator=(GcRooted&& o) noexcept;
  ~GcRooted();

  GcPtr<T> get() const noexcept { return GcPtr<T>(slot_, ptr_); }

 private:
  friend class GcHeap;
  GcRooted(GcHeap* heap, GcSlot* slot, T* p) noexcept
      : heap_(heap), slot_(slot), ptr_(p) {}
  void release() noexcept;

  GcHeap* heap_ = nullptr;
  GcSlot* slot_ = nullptr;
  T* ptr_ = nullptr;
};

// Classic mark-sweep heap: the ex08-pool-shaped slot store grown a tracing
// collector. No reference counts anywhere — cycles are collected like
// everything else, which is the whole point next to ex51's refcounting.
//
// Contract:
//   - allocate<T>(args...) creates an object, returns a non-owning handle.
//   - GcRooted<T> is the move-only RAII root guard.
//   - collect() marks from the roots (following trace() edges) and sweeps
//     everything unmarked, anywhere in the heap; returns the freed count.
//   - Cycles with no live root are reclaimed (unlike refcounting).
//   - live()/roots() introspect; is_live(h) reports survival.
class GcHeap {
 public:
  GcHeap() = default;
  GcHeap(const GcHeap&) = delete;
  GcHeap& operator=(const GcHeap&) = delete;

  template <typename T, typename... Args>
  GcPtr<T> allocate(Args&&... args) {
    auto slot = std::make_unique<GcSlotFor<T>>(std::forward<Args>(args)...);
    T* p = &slot->obj_;
    GcSlot* raw = slot.get();
    slots_.push_back(std::move(slot));
    live_.insert(raw);
    return GcPtr<T>(raw, p);
  }

  template <typename T>
  GcRooted<T> root(GcPtr<T> h) {
    if (h.slot_ptr() != nullptr) roots_.push_back(h.slot_ptr());
    return GcRooted<T>(this, h.slot_ptr(), h.get());
  }

  std::size_t collect() {
    // Mark: iterative worklist from the roots.
    std::vector<GcSlot*> stack = roots_;
    while (!stack.empty()) {
      GcSlot* s = stack.back();
      stack.pop_back();
      if (s == nullptr || live_.find(s) == live_.end()) continue;
      if (s->marked) continue;
      s->marked = true;
      GcTracer tr(&stack);
      s->trace(tr);
    }
    // Sweep: erase every unmarked slot, wherever it sits.
    std::size_t freed = 0;
    for (auto it = slots_.begin(); it != slots_.end();) {
      if (!(*it)->marked) {
        live_.erase(it->get());
        it = slots_.erase(it);  // destroys the object
        ++freed;
      } else {
        (*it)->marked = false;
        ++it;
      }
    }
    return freed;
  }

  template <typename T>
  bool is_live(GcPtr<T> h) const noexcept {
    return h.slot_ptr() != nullptr && live_.find(h.slot_ptr()) != live_.end();
  }

  std::size_t live() const noexcept { return live_.size(); }
  std::size_t roots() const noexcept { return roots_.size(); }

 private:
  template <typename U>
  friend class GcRooted;
  void unroot(GcSlot* s) noexcept {
    for (std::size_t i = 0; i < roots_.size(); ++i) {
      if (roots_[i] == s) {
        roots_[i] = roots_.back();
        roots_.pop_back();
        return;
      }
    }
  }

  std::list<std::unique_ptr<GcSlot>> slots_;
  std::unordered_set<GcSlot*> live_;
  std::vector<GcSlot*> roots_;
};

template <typename T>
GcRooted<T>& GcRooted<T>::operator=(GcRooted<T>&& o) noexcept {
  if (this != &o) {
    release();
    heap_ = o.heap_;
    slot_ = o.slot_;
    ptr_ = o.ptr_;
    o.heap_ = nullptr;
    o.slot_ = nullptr;
    o.ptr_ = nullptr;
  }
  return *this;
}

template <typename T>
GcRooted<T>::~GcRooted() {
  release();
}

template <typename T>
void GcRooted<T>::release() noexcept {
  if (heap_ != nullptr && slot_ != nullptr) {
    heap_->unroot(slot_);
    heap_ = nullptr;
    slot_ = nullptr;
    ptr_ = nullptr;
  }
}

#endif  // EXERCISE50_MARK_SWEEP_H_
```
