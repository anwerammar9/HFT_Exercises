# Exercise memory/04_shared_ptr (ex45) — Shared Pointer (Reference Solution)

**What you implement:** shared ownership (`SharedPtr`, a `std::shared_ptr`
clone) over a type-erased, atomically refcounted control block, plus the
non-owning `WeakPtr` observer that breaks reference cycles, custom-deleter
support, an aliasing constructor, and the single-allocation `MakeShared`
factory.

**Approach**
- One `ControlBase` per object: `atomic<size_t> shared_/weak_` plus a
  virtual `dispose()` (object destruction only — the block itself is freed
  by `release_weak`). `weak_` starts at **1**: the shared owners' own stake
  in the block, so the block dies exactly when the last owner *and* the last
  weak are gone (the libstdc++ two-counter shape).
- `release_shared`: `fetch_sub(acq_rel)`; on last, `dispose()` then drop the
  stake. `add_shared`/`add_weak` are relaxed bumps. `lock()` upgrades via
  `try_add_shared()`, a CAS loop that claims a ref iff the count is nonzero —
  the lost race against the last release just yields an empty `SharedPtr`.
- `ControlWithDeleter<T, D>` pairs the raw pointer with any deleter;
  `ControlInline<T>` holds `alignas(T)` inline bytes so `MakeShared`
  placement-news the object into the *same* allocation as the block (and
  plain-`delete`s the block if construction throws, before any `dispose`).
- Copies bump in the member init list; moves steal and null the source;
  assignment is copy/move-and-swap, which makes self-assign (copy *and* move)
  trivially safe. Converting copies/moves and the aliasing constructor are
  `convertible_to`-constrained templates built on friendship between
  instantiations.
- Thread rule (same as std): concurrent copies of *distinct* `SharedPtr`
  objects are safe; touching the *same* `SharedPtr` object from two threads
  is a data race.

## Reference API + implementation — `include/shared_ptr.h`

The classes are templates, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE45_SHARED_PTR_H_
#define EXERCISE45_SHARED_PTR_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <typename T>
class SharedPtr;
template <typename T>
class WeakPtr;

// Type-erased control block: one per managed object, shared by every
// SharedPtr/WeakPtr that co-owns it. The block outlives the object whenever a
// WeakPtr is watching (the classic "weak keeps the block, shared keeps the
// object" split).
class ControlBase {
 public:
  ControlBase() : shared_(1), weak_(1) {}  // weak_ starts at 1: the shared
                                           // owners' own stake in the block
  virtual ~ControlBase() = default;

  virtual void dispose() noexcept = 0;  // destroy the managed object only

  void add_shared() noexcept {
    shared_.fetch_add(1, std::memory_order_relaxed);
  }
  void release_shared() noexcept {
    if (shared_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      dispose();        // last owner: destroy the object...
      release_weak();   // ...then drop the owners' stake in the block
    }
  }
  void add_weak() noexcept { weak_.fetch_add(1, std::memory_order_relaxed); }
  void release_weak() noexcept {
    if (weak_.fetch_sub(1, std::memory_order_acq_rel) == 1) delete this;
  }
  long shared_count() const noexcept {
    return static_cast<long>(shared_.load(std::memory_order_acquire));
  }

  // Claim one shared ref iff the object is still alive (lock()'s upgrade).
  bool try_add_shared() noexcept {
    std::size_t cur = shared_.load(std::memory_order_acquire);
    while (cur != 0) {
      if (shared_.compare_exchange_weak(cur, cur + 1,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire))
        return true;
      // On failure cur refreshes; loop retries unless we raced the last
      // release (cur == 0 -> the object is gone).
    }
    return false;
  }

 private:
  std::atomic<std::size_t> shared_;
  std::atomic<std::size_t> weak_;
};

// Control block for a separately-allocated object with a custom deleter.
template <typename T, typename Deleter>
class ControlWithDeleter : public ControlBase {
 public:
  ControlWithDeleter(T* p, Deleter d) : object_(p), deleter_(std::move(d)) {}
  void dispose() noexcept override { deleter_(object_); }

 private:
  T* object_;
  Deleter deleter_;
};

// Control block with inline object storage: MakeShared's single allocation.
template <typename T>
class ControlInline : public ControlBase {
 public:
  ControlInline() = default;

  template <typename... Args>
  void construct(Args&&... args) {
    new (storage()) T(std::forward<Args>(args)...);
  }
  T* object() noexcept { return reinterpret_cast<T*>(storage()); }

  void dispose() noexcept override { object()->~T(); }

 private:
  void* storage() noexcept { return static_cast<void*>(storage_); }
  alignas(T) unsigned char storage_[sizeof(T)];
};

struct AdoptTag {};

// Shared ownership (a std::shared_ptr clone).
//
// Contract:
//   - Copies share ownership (atomic refcount +1); moves steal without
//     touching the count; the last SharedPtr destroys the object.
//   - use_count() reports live co-owners (0 when empty); unique() is
//     use_count() == 1. All count operations are atomic: concurrent copies
//     of *distinct* SharedPtr objects (same ownership) are safe.
//   - WeakPtr observes without owning; lock() upgrades iff the object is
//     still alive (CAS loop on the count); the parent/child cycle test
//     proves weak breaks reference cycles.
//   - Custom deleters via SharedPtr(p, deleter); the aliasing constructor
//     shares ownership while pointing elsewhere (member views).
//   - MakeShared<T>(args...) builds object + block in ONE allocation.
template <typename T>
class SharedPtr {
 public:
  using element_type = T;

  constexpr SharedPtr() noexcept : ptr_(nullptr), ctrl_(nullptr) {}
  constexpr SharedPtr(std::nullptr_t) noexcept : SharedPtr() {}

  explicit SharedPtr(T* p)
      : ptr_(p),
        ctrl_(new ControlWithDeleter<T, std::default_delete<T>>(p, {})) {}

  template <typename Deleter>
  SharedPtr(T* p, Deleter d)
      : ptr_(p), ctrl_(new ControlWithDeleter<T, Deleter>(p, std::move(d))) {}

  ~SharedPtr() {
    if (ctrl_ != nullptr) ctrl_->release_shared();
  }

  SharedPtr(const SharedPtr& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_shared();
  }
  SharedPtr(SharedPtr&& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    o.ptr_ = nullptr;
    o.ctrl_ = nullptr;
  }

  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr(const SharedPtr<U>& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_shared();
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr(SharedPtr<U>&& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    o.ptr_ = nullptr;
    o.ctrl_ = nullptr;
  }

  // Aliasing: share o's ownership while pointing at p (e.g. a member).
  template <typename U>
  SharedPtr(const SharedPtr<U>& o, T* p) noexcept : ptr_(p), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_shared();
  }

  SharedPtr& operator=(const SharedPtr& o) noexcept {
    SharedPtr(o).swap(*this);
    return *this;
  }
  SharedPtr& operator=(SharedPtr&& o) noexcept {
    SharedPtr(std::move(o)).swap(*this);  // self-move safe: steal then swap back
    return *this;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr& operator=(const SharedPtr<U>& o) noexcept {
    SharedPtr(o).swap(*this);
    return *this;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr& operator=(SharedPtr<U>&& o) noexcept {
    SharedPtr(std::move(o)).swap(*this);
    return *this;
  }

  void reset() noexcept { SharedPtr().swap(*this); }
  void reset(T* p) { SharedPtr(p).swap(*this); }
  template <typename Deleter>
  void reset(T* p, Deleter d) {
    SharedPtr(p, std::move(d)).swap(*this);
  }

  void swap(SharedPtr& o) noexcept {
    using std::swap;
    swap(ptr_, o.ptr_);
    swap(ctrl_, o.ctrl_);
  }

  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  long use_count() const noexcept {
    return ctrl_ != nullptr ? ctrl_->shared_count() : 0;
  }
  bool unique() const noexcept { return use_count() == 1; }

 private:
  template <typename U>
  friend class SharedPtr;
  template <typename U>
  friend class WeakPtr;
  template <typename U, typename... Args>
  friend SharedPtr<U> MakeShared(Args&&...);

  SharedPtr(T* p, ControlBase* c, AdoptTag) noexcept : ptr_(p), ctrl_(c) {}

  T* ptr_;
  ControlBase* ctrl_;
};

// Non-owning observer (a std::weak_ptr clone): breaks reference cycles.
template <typename T>
class WeakPtr {
 public:
  constexpr WeakPtr() noexcept : ptr_(nullptr), ctrl_(nullptr) {}

  WeakPtr(const SharedPtr<T>& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_weak();
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  WeakPtr(const SharedPtr<U>& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_weak();
  }

  WeakPtr(const WeakPtr& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_weak();
  }
  WeakPtr(WeakPtr&& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    o.ptr_ = nullptr;
    o.ctrl_ = nullptr;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  WeakPtr(const WeakPtr<U>& o) noexcept : ptr_(o.ptr_), ctrl_(o.ctrl_) {
    if (ctrl_ != nullptr) ctrl_->add_weak();
  }

  ~WeakPtr() {
    if (ctrl_ != nullptr) ctrl_->release_weak();
  }

  WeakPtr& operator=(const WeakPtr& o) noexcept {
    WeakPtr(o).swap(*this);
    return *this;
  }
  WeakPtr& operator=(WeakPtr&& o) noexcept {
    WeakPtr(std::move(o)).swap(*this);
    return *this;
  }
  WeakPtr& operator=(const SharedPtr<T>& o) noexcept {
    WeakPtr(o).swap(*this);
    return *this;
  }

  void reset() noexcept { WeakPtr().swap(*this); }
  void swap(WeakPtr& o) noexcept {
    using std::swap;
    swap(ptr_, o.ptr_);
    swap(ctrl_, o.ctrl_);
  }

  long use_count() const noexcept {
    return ctrl_ != nullptr ? ctrl_->shared_count() : 0;
  }
  bool expired() const noexcept { return use_count() == 0; }

  // Upgrade to shared ownership, or empty if the object is already gone.
  SharedPtr<T> lock() const noexcept {
    if (ctrl_ != nullptr && ctrl_->try_add_shared())
      return SharedPtr<T>(ptr_, ctrl_, AdoptTag{});
    return SharedPtr<T>();
  }

 private:
  template <typename U>
  friend class WeakPtr;

  T* ptr_;
  ControlBase* ctrl_;
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  auto* c = new ControlInline<T>();
  try {
    c->construct(std::forward<Args>(args)...);
  } catch (...) {
    delete c;
    throw;
  }
  return SharedPtr<T>(c->object(), c, AdoptTag{});
}

template <typename T>
void swap(SharedPtr<T>& a, SharedPtr<T>& b) noexcept {
  a.swap(b);
}
template <typename T>
void swap(WeakPtr<T>& a, WeakPtr<T>& b) noexcept {
  a.swap(b);
}

template <typename T>
bool operator==(const SharedPtr<T>& a, const SharedPtr<T>& b) noexcept {
  return a.get() == b.get();
}
template <typename T>
bool operator!=(const SharedPtr<T>& a, const SharedPtr<T>& b) noexcept {
  return !(a == b);
}
template <typename T>
bool operator==(const SharedPtr<T>& p, std::nullptr_t) noexcept {
  return !p;
}
template <typename T>
bool operator==(std::nullptr_t, const SharedPtr<T>& p) noexcept {
  return !p;
}
template <typename T>
bool operator!=(const SharedPtr<T>& p, std::nullptr_t) noexcept {
  return static_cast<bool>(p);
}
template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T>& p) noexcept {
  return static_cast<bool>(p);
}

#endif  // EXERCISE45_SHARED_PTR_H_
```
