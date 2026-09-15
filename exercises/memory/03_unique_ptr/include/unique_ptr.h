#ifndef EXERCISE44_UNIQUE_PTR_H_
#define EXERCISE44_UNIQUE_PTR_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// TODO(anwer): implement exclusive-ownership move semantics (see SOLUTION.md).
//
// Contract:
//   - Exactly one UniquePtr owns the object; ownership moves, never copies.
//   - The destructor destroys via the deleter; release() relinquishes without
//     destroying; reset(p) destroys the old object and owns p.
//   - Move ctor/assign transfer ownership and empty the source; self-move is
//     a safe no-op; converting moves allow Derived -> Base.
//   - The T[] specialization manages arrays (delete[], operator[]).
//   - MakeUnique<T>(args...) / MakeUniqueArray<T>(n[, value]) are factories.
//
// Suggested shape (see SOLUTION.md):
//   - Two members: T* ptr_ + Deleter deleter_.
//   - Dtor = reset(); move = steal + null the source; reset = stash old,
//     store new, destroy old; release = stash, store null, return stashed.
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

  UniquePtr(UniquePtr&& o) noexcept // TODO(anwer): steal + null the source
      : ptr_(nullptr), deleter_() {
    (void)o;
  }

  UniquePtr& operator=(UniquePtr&& o) noexcept {  // TODO(anwer): see above
    (void)o;
    return *this;
  }

  template <typename U, typename E>
    requires(std::convertible_to<U*, T*> && std::is_constructible_v<Deleter, E&&>)
  UniquePtr(UniquePtr<U, E>&& o)  // TODO(anwer): converting move
      : ptr_(nullptr), deleter_() {
    (void)o;
  }

  template <typename U, typename E>
    requires(std::convertible_to<U*, T*> && std::is_assignable_v<Deleter&, E&&>)
  UniquePtr& operator=(UniquePtr<U, E>&& o) {  // TODO(anwer): converting move
    (void)o;
    return *this;
  }

  T* get() const noexcept { return ptr_; }
  const Deleter& get_deleter() const noexcept { return deleter_; }
  Deleter& get_deleter() noexcept { return deleter_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }

  T* release() noexcept {  // TODO(anwer): relinquish without destroying
    return nullptr;
  }

  void reset(T* p = nullptr) noexcept {  // TODO(anwer): destroy old, own p
    if (p == nullptr) {
      T* old = ptr_;
      ptr_ = nullptr;
      if (old != nullptr) deleter_(old);
    } else {
      deleter_(p);  // stub drops the replacement instead of owning it
    }
  }

  void swap(UniquePtr& o) noexcept {  // TODO(anwer): exchange ownership
    (void)o;
  }

 private:
  T* ptr_;
  Deleter deleter_;
};

// TODO(anwer): array specialization (see SOLUTION.md).
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

  UniquePtr(UniquePtr&& o) noexcept  // TODO(anwer): steal + null the source
      : ptr_(nullptr), deleter_() {
    (void)o;
  }

  UniquePtr& operator=(UniquePtr&& o) noexcept {  // TODO(anwer): see above
    (void)o;
    return *this;
  }

  T* get() const noexcept { return ptr_; }
  const Deleter& get_deleter() const noexcept { return deleter_; }
  Deleter& get_deleter() noexcept { return deleter_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T& operator[](std::size_t i) const noexcept {  // TODO(anwer): index i
    (void)i;
    return ptr_[0];
  }

  T* release() noexcept {  // TODO(anwer): relinquish without destroying
    return nullptr;
  }

  void reset(T* p = nullptr) noexcept {  // TODO(anwer): destroy old, own p
    if (p == nullptr) {
      T* old = ptr_;
      ptr_ = nullptr;
      if (old != nullptr) deleter_(old);
    } else {
      deleter_(p);  // stub drops the replacement instead of owning it
    }
  }

  void swap(UniquePtr& o) noexcept {  // TODO(anwer): exchange ownership
    (void)o;
  }

 private:
  T* ptr_;
  Deleter deleter_;
};

// TODO(anwer): exception-safe factories (see SOLUTION.md).
template <typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args) {
  (void)sizeof...(args);
  return UniquePtr<T>();
}

template <typename T>
UniquePtr<T[]> MakeUniqueArray(std::size_t n) {
  (void)n;
  return UniquePtr<T[]>();
}

template <typename T>
UniquePtr<T[]> MakeUniqueArray(std::size_t n, const T& value) {
  (void)n;
  (void)value;
  return UniquePtr<T[]>();
}

template <typename T, typename D>
void swap(UniquePtr<T, D>& a, UniquePtr<T, D>& b) noexcept {
  (void)a;
  (void)b;
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
