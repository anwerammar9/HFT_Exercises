# Exercise memory/08_arena_gc (ex49) — Arena Garbage Collector (Reference Solution)

**What you implement:** the ex07 arena grown a mark phase — bump-order slots,
explicit roots, `trace()` edges, and a `collect()` that pops only the *dead
tail*. Interior garbage stays pinned (no moving objects), and `reset()` keeps
its bulk-reclaim role.

**Approach**
- Slots: `vector<unique_ptr<ArenaGcSlot>>` in push-back (bump) order; roots:
  `vector<size_t>` indices. `ArenaGcSlotFor<T>` holds the object and calls
  `trace()` only `if constexpr (ArenaTraceable<T>)` (leaves need no method).
- `collect()`: iterative index worklist from the roots (skip the null
  sentinel, out-of-range, and already-marked) → pop the dead tail (`while`
  the back is unmarked) → unmark survivors → prune root indices past the new
  end. Survivor indices never shift (only the tail pops), so handles and
  roots stay valid.
- `root()` pushes the index; `ArenaRoot`'s dtor/move removes exactly one
  registration (`unroot`); rooting a null handle is inert (the mark skips the
  sentinel, the prune drops it).
- Handles are non-owning indices; `is_live()` compares against the size, so
  it never dereferences freed memory. Capacity bounds allocation (null handle
  when full). Single-threaded by contract.

## Reference API + implementation — `include/arena_gc.h`

The classes are templates, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE49_ARENA_GC_H_
#define EXERCISE49_ARENA_GC_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class ArenaPtr;
template <typename T>
class ArenaRoot;

// Forward declaration for the trace concept below.
class ArenaGcHeap;

// Outgoing edges are declared by the payload itself: define
//   void trace(ArenaGcHeap::Tracer& tr) const;
// and the collector follows them. Payloads without trace() are leaves.
struct ArenaGcTracer {
  explicit ArenaGcTracer(std::vector<std::size_t>* work) : work_(work) {}
  void visit(std::size_t slot) {
    if (slot != kNull) work_->push_back(slot);
  }
  static constexpr std::size_t kNull = static_cast<std::size_t>(-1);

 private:
  std::vector<std::size_t>* work_;
};

template <typename T>
concept ArenaTraceable = requires(const T& t, ArenaGcTracer& tr) {
  t.trace(tr);
};

// Type-erased bump slot: allocated in push-back order, reclaimed from the
// tail (or all at once via reset).
class ArenaGcSlot {
 public:
  virtual ~ArenaGcSlot() = default;
  virtual void trace(ArenaGcTracer& tr) const = 0;
  bool marked = false;
};

template <typename T>
class ArenaGcSlotFor : public ArenaGcSlot {
 public:
  template <typename... Args>
  explicit ArenaGcSlotFor(Args&&... args) : obj_(std::forward<Args>(args)...) {}
  void trace(ArenaGcTracer& tr) const override {
    if constexpr (ArenaTraceable<T>) obj_.trace(tr);
  }
  T obj_;
};

// Bump-arena garbage collector: the ex07 arena grown a mark phase. Allocation
// pushes slots in order; collection marks from the roots, then pops the DEAD
// TAIL only — interior garbage stays pinned until everything above it dies
// (the fragmentation cost of never moving objects). reset() is the bulk
// reclaim escape hatch.
//
// Contract:
//   - allocate<T>(args...) appends a slot and returns a non-owning handle;
//     null handle when capacity is exhausted.
//   - ArenaRoot<T> is the RAII root guard (move-only): live while guarded.
//   - collect() marks from roots, pops the dead tail, returns popped count.
//   - Interior dead objects are NOT reclaimed (pinned until tail drains).
//   - live()/capacity()/used() introspect; reset() frees everything but keeps
//     the capacity.
//   - Handles are non-owning: is_live(h) tells whether collect() took the
//     object; never dereference a dead handle.
template <typename T>
class ArenaPtr {
 public:
  ArenaPtr() noexcept : heap_(nullptr), slot_(ArenaGcTracer::kNull), ptr_(nullptr) {}
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(const ArenaPtr& o) const noexcept { return ptr_ == o.ptr_; }

  // Public so payload trace() methods can name their edges
  // (tr.visit(child.slot())); not for general use.
  std::size_t slot() const noexcept { return slot_; }

 private:
  friend class ArenaGcHeap;
  template <typename U>
  friend class ArenaRoot;
  ArenaPtr(ArenaGcHeap* heap, std::size_t slot, T* p) noexcept
      : heap_(heap), slot_(slot), ptr_(p) {}

  ArenaGcHeap* heap_;
  std::size_t slot_;
  T* ptr_;
};

template <typename T>
class ArenaRoot {
 public:
  ArenaRoot() noexcept = default;
  ArenaRoot(const ArenaRoot&) = delete;
  ArenaRoot& operator=(const ArenaRoot&) = delete;
  ArenaRoot(ArenaRoot&& o) noexcept
      : heap_(o.heap_), slot_(o.slot_), ptr_(o.ptr_) {
    o.heap_ = nullptr;
    o.slot_ = ArenaGcTracer::kNull;
    o.ptr_ = nullptr;
  }
  ArenaRoot& operator=(ArenaRoot&& o) noexcept;
  ~ArenaRoot();

  ArenaPtr<T> get() const noexcept { return ArenaPtr<T>(heap_, slot_, ptr_); }

 private:
  friend class ArenaGcHeap;
  ArenaRoot(ArenaGcHeap* heap, std::size_t slot, T* p) noexcept
      : heap_(heap), slot_(slot), ptr_(p) {}
  void release() noexcept;

  ArenaGcHeap* heap_ = nullptr;
  std::size_t slot_ = ArenaGcTracer::kNull;
  T* ptr_ = nullptr;
};

class ArenaGcHeap {
 public:
  using Tracer = ArenaGcTracer;

  explicit ArenaGcHeap(std::size_t capacity) : capacity_(capacity) {}

  ArenaGcHeap(const ArenaGcHeap&) = delete;
  ArenaGcHeap& operator=(const ArenaGcHeap&) = delete;

  template <typename T, typename... Args>
  ArenaPtr<T> allocate(Args&&... args) {
    if (slots_.size() >= capacity_) return ArenaPtr<T>();
    auto slot = std::make_unique<ArenaGcSlotFor<T>>(std::forward<Args>(args)...);
    T* p = &slot->obj_;
    const std::size_t idx = slots_.size();
    slots_.push_back(std::move(slot));
    return ArenaPtr<T>(this, idx, p);
  }

  template <typename T>
  ArenaRoot<T> root(ArenaPtr<T> h) {
    roots_.push_back(h.slot());
    return ArenaRoot<T>(this, h.slot(), h.get());
  }

  // Mark from roots; pop the dead tail; unmark survivors. Returns reclaimed.
  std::size_t collect() {
    // Mark: iterative worklist over slot indices.
    std::vector<std::size_t> stack = roots_;
    while (!stack.empty()) {
      const std::size_t i = stack.back();
      stack.pop_back();
      if (i == Tracer::kNull || i >= slots_.size()) continue;
      ArenaGcSlot* s = slots_[i].get();
      if (s->marked) continue;
      s->marked = true;
      Tracer tr(&stack);
      s->trace(tr);
    }
    // Sweep the tail only: pop dead slots until live rock (or empty).
    std::size_t freed = 0;
    while (!slots_.empty() && !slots_.back()->marked) {
      slots_.pop_back();
      ++freed;
    }
    for (auto& s : slots_) s->marked = false;
    // Roots pointing past the new end dangle by index; drop them.
    std::size_t kept = 0;
    for (std::size_t r : roots_) {
      if (r < slots_.size()) roots_[kept++] = r;
    }
    roots_.resize(kept);
    return freed;
  }

  bool is_live(std::size_t slot) const noexcept { return slot < slots_.size(); }
  template <typename T>
  bool is_live(ArenaPtr<T> h) const noexcept {
    return h.slot() < slots_.size();
  }

  std::size_t live() const noexcept { return slots_.size(); }
  std::size_t capacity() const noexcept { return capacity_; }
  std::size_t roots() const noexcept { return roots_.size(); }

  void reset() {
    slots_.clear();
    roots_.clear();
  }

 private:
  template <typename U>
  friend class ArenaRoot;
  void unroot(std::size_t slot) noexcept {
    for (std::size_t i = 0; i < roots_.size(); ++i) {
      if (roots_[i] == slot) {
        roots_[i] = roots_.back();
        roots_.pop_back();
        return;
      }
    }
  }

  std::vector<std::unique_ptr<ArenaGcSlot>> slots_;
  std::vector<std::size_t> roots_;
  std::size_t capacity_;
};

template <typename T>
ArenaRoot<T>& ArenaRoot<T>::operator=(ArenaRoot<T>&& o) noexcept {
  if (this != &o) {
    release();
    heap_ = o.heap_;
    slot_ = o.slot_;
    ptr_ = o.ptr_;
    o.heap_ = nullptr;
    o.slot_ = ArenaGcTracer::kNull;
    o.ptr_ = nullptr;
  }
  return *this;
}

template <typename T>
ArenaRoot<T>::~ArenaRoot() {
  release();
}

template <typename T>
void ArenaRoot<T>::release() noexcept {
  if (heap_ != nullptr && slot_ != ArenaGcTracer::kNull) {
    heap_->unroot(slot_);
    heap_ = nullptr;
    slot_ = ArenaGcTracer::kNull;
    ptr_ = nullptr;
  }
}

#endif  // EXERCISE49_ARENA_GC_H_
```
