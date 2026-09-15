# Exercise memory/03_unique_ptr (ex44) — Unique Pointer (Reference Solution)

**What you implement:** an exclusive-ownership smart pointer (`UniquePtr`,
a `std::unique_ptr` clone) with move-only lifetime, custom-deleter support,
a Derived → Base converting move, an array (`T[]`) partial specialization,
and exception-safe `MakeUnique` / `MakeUniqueArray` factories.

**Approach**
- Two members, `T* ptr_` + `Deleter deleter_`; every operation is a few lines
  (this exercise is about getting the *rules* exactly right, not machinery).
- Destructor = `reset()` (destroy old via the deleter). Move ctor steals the
  pointer, moves the deleter, and nulls the source. Move assign guards
  `this != &o` (that guard is what makes self-move-assign a safe no-op),
  then `reset(o.release())` plus deleter move-assign.
- `release()` stashes, stores null, returns stashed — no destroy.
  `reset(p)` stashes old, stores new, destroys old.
- Converting moves are templates constrained on
  `convertible_to<U*, T*>` plus deleter constructibility/assignability, and
  are built only from the public `release()` / `get_deleter()` — no
  friendship needed. The default deleter for `UniquePtr<T[]>` falls out of
  the primary's default (`std::default_delete<T>` with `T = U[]`).
- Factories keep naked `new` out of call sites: `MakeUnique<T>(args...)`
  perfect-forwards; `MakeUniqueArray<T>(n)` value-initializes; the
  `(n, value)` overload fills.
- `noexcept` is conditional on the deleter's move operations (a throwing
  deleter move must not `terminate` inside a move).

## Reference API + implementation — `include/unique_ptr.h`

The class is a template, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE44_UNIQUE_PTR_H_
#define EXERCISE44_UNIQUE_PTR_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// Exclusive-ownership smart pointer (a std::unique_ptr clone).
//
// Contract:
//   - Exactly one UniquePtr owns the object; ownership moves, never copies
//     (copy ctor/assign are deleted).
//   - The destructor destroys the owned object via the deleter
//     (`Deleter`, default `std::default_delete<T>`).
//   - get()/release()/reset()/swap() mirror std::unique_ptr semantics:
//     release() relinquishes without destroying; reset(p) destroys the old
//     object and takes ownership of p (p == nullptr by default).
//   - Move construction/assignment transfer ownership and leave the source
//     empty; self-move-assign is a safe no-op.
//   - Converting moves from UniquePtr<U, E> are allowed when U* converts to
//     T* and the deleter converts (the Derived -> Base interview case).
//   - The T[] partial specialization manages arrays (delete[], operator[]
//     instead of operator*/->).
//   - MakeUnique<T>(args...) / MakeUniqueArray<T>(n[, value]) are the
//     exception-safe factories (no naked new at the call site).
template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
 public:
  using element_type = T;
  using deleter_type = Deleter;
  using pointer = T*;

  constexpr UniquePtr() noexcept : ptr_(nullptr), deleter_() {}
  constexpr UniquePtr(std::nullptr_t) noexcept : UniquePtr() {}

  explicit UniquePtr(T* p) noexcept : ptr_(p), deleter_() {}
  UniquePtr(T* p, const Deleter& d) : ptr_(p), deleter_(d) {}
  UniquePtr(T* p, Deleter&& d) : ptr_(p), deleter_(std::move(d)) {}

  ~UniquePtr() { reset(); }

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  UniquePtr(UniquePtr&& o) noexcept(std::is_nothrow_move_constructible_v<Deleter>)
      : ptr_(o.ptr_), deleter_(std::move(o.deleter_)) {
    o.ptr_ = nullptr;
  }

  UniquePtr& operator=(UniquePtr&& o) noexcept(
      std::is_nothrow_move_assignable_v<Deleter>) {
    if (this != &o) {
      reset(o.release());
      deleter_ = std::move(o.deleter_);
    }
    return *this;
  }

  template <typename U, typename E>
    requires(std::convertible_to<U*, T*> && std::is_constructible_v<Deleter, E&&>)
  UniquePtr(UniquePtr<U, E>&& o) noexcept(
      std::is_nothrow_constructible_v<Deleter, E&&>)
      : ptr_(o.release()), deleter_(std::forward<E>(o.get_deleter())) {}

  template <typename U, typename E>
    requires(std::convertible_to<U*, T*> && std::is_assignable_v<Deleter&, E&&>)
  UniquePtr& operator=(UniquePtr<U, E>&& o) noexcept(
      std::is_nothrow_assignable_v<Deleter&, E&&>) {
    reset(o.release());
    deleter_ = std::forward<E>(o.get_deleter());
    return *this;
  }

  T* get() const noexcept { return ptr_; }
  const Deleter& get_deleter() const noexcept { return deleter_; }
  Deleter& get_deleter() noexcept { return deleter_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }

  T* release() noexcept {
    T* p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  void reset(T* p = nullptr) noexcept {
    T* old = ptr_;
    ptr_ = p;
    if (old != nullptr) deleter_(old);
  }

  void swap(UniquePtr& o) noexcept {
    using std::swap;
    swap(ptr_, o.ptr_);
    swap(deleter_, o.deleter_);
  }

 private:
  T* ptr_;
  Deleter deleter_;
};

// Array specialization: delete[] ownership with indexed access.
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
 public:
  using element_type = T;
  using deleter_type = Deleter;
  using pointer = T*;

  constexpr UniquePtr() noexcept : ptr_(nullptr), deleter_() {}
  constexpr UniquePtr(std::nullptr_t) noexcept : UniquePtr() {}

  explicit UniquePtr(T* p) noexcept : ptr_(p), deleter_() {}
  UniquePtr(T* p, const Deleter& d) : ptr_(p), deleter_(d) {}
  UniquePtr(T* p, Deleter&& d) : ptr_(p), deleter_(std::move(d)) {}

  ~UniquePtr() { reset(); }

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  UniquePtr(UniquePtr&& o) noexcept(std::is_nothrow_move_constructible_v<Deleter>)
      : ptr_(o.ptr_), deleter_(std::move(o.deleter_)) {
    o.ptr_ = nullptr;
  }

  UniquePtr& operator=(UniquePtr&& o) noexcept(
      std::is_nothrow_move_assignable_v<Deleter>) {
    if (this != &o) {
      reset(o.release());
      deleter_ = std::move(o.deleter_);
    }
    return *this;
  }

  T* get() const noexcept { return ptr_; }
  const Deleter& get_deleter() const noexcept { return deleter_; }
  Deleter& get_deleter() noexcept { return deleter_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T& operator[](std::size_t i) const noexcept { return ptr_[i]; }

  T* release() noexcept {
    T* p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  void reset(T* p = nullptr) noexcept {
    T* old = ptr_;
    ptr_ = p;
    if (old != nullptr) deleter_(old);
  }

  void swap(UniquePtr& o) noexcept {
    using std::swap;
    swap(ptr_, o.ptr_);
    swap(deleter_, o.deleter_);
  }

 private:
  T* ptr_;
  Deleter deleter_;
};

template <typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args) {
  return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
UniquePtr<T[]> MakeUniqueArray(std::size_t n) {
  return UniquePtr<T[]>(new T[n]());
}

template <typename T>
UniquePtr<T[]> MakeUniqueArray(std::size_t n, const T& value) {
  UniquePtr<T[]> p(new T[n]);
  for (std::size_t i = 0; i < n; ++i) p[i] = value;
  return p;
}

template <typename T, typename D>
void swap(UniquePtr<T, D>& a, UniquePtr<T, D>& b) noexcept {
  a.swap(b);
}

template <typename T, typename D>
bool operator==(const UniquePtr<T, D>& p, std::nullptr_t) noexcept {
  return !p;
}
template <typename T, typename D>
bool operator==(std::nullptr_t, const UniquePtr<T, D>& p) noexcept {
  return !p;
}
template <typename T, typename D>
bool operator!=(const UniquePtr<T, D>& p, std::nullptr_t) noexcept {
  return static_cast<bool>(p);
}
template <typename T, typename D>
bool operator!=(std::nullptr_t, const UniquePtr<T, D>& p) noexcept {
  return static_cast<bool>(p);
}

#endif  // EXERCISE44_UNIQUE_PTR_H_
```
