#ifndef EXERCISE45_SHARED_PTR_H_
#define EXERCISE45_SHARED_PTR_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// TODO(anwer): implement atomic shared ownership + weak observation
// (see SOLUTION.md).
//
// Contract:
//   - Copies share ownership (atomic +1); moves steal; the last SharedPtr
//     destroys the object. use_count()/unique() report co-owners.
//   - WeakPtr observes without owning; lock() upgrades iff alive; weak
//     breaks reference cycles (parent/child test).
//   - Custom deleters run once on last release; the aliasing ctor shares
//     ownership while pointing elsewhere; MakeShared is one allocation.
//
// Suggested shape (see SOLUTION.md):
//   - ControlBase { atomic shared/weak (weak starts at 1: the owners' stake),
//     virtual dispose(), try_add_shared() CAS loop }.
//   - ControlWithDeleter<T, D> (raw ptr + deleter); ControlInline<T>
//     (alignas(T) inline bytes for MakeShared).
//   - SharedPtr { T* ptr_; ControlBase* ctrl_; }; copies add_shared, moves
//     steal + null, assignment is copy/move-and-swap (self-safe).
template <typename T>
class SharedPtr;
template <typename T>
class WeakPtr;

class ControlBase {
 public:
  ControlBase() : shared_(1), weak_(1) {}
  virtual ~ControlBase() = default;

  virtual void dispose() noexcept = 0;  // TODO(anwer): destroy the object

  void add_shared() noexcept {  // TODO(anwer): atomic +1
    (void)shared_;
  }
  void release_shared() noexcept {  // TODO(anwer): last release disposes
  }
  void add_weak() noexcept {  // TODO(anwer): atomic +1
    (void)weak_;
  }
  void release_weak() noexcept {  // TODO(anwer): last weak deletes the block
  }
  long shared_count() const noexcept {  // TODO(anwer): atomic load
    return 0;
  }
  bool try_add_shared() noexcept {  // TODO(anwer): CAS upgrade iff alive
    return false;
  }

 private:
  std::atomic<std::size_t> shared_;
  std::atomic<std::size_t> weak_;
};

template <typename T, typename Deleter>
class ControlWithDeleter : public ControlBase {
 public:
  ControlWithDeleter(T* p, Deleter d) : object_(p), deleter_(std::move(d)) {}
  void dispose() noexcept override { deleter_(object_); }

 private:
  T* object_;
  Deleter deleter_;
};

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

template <typename T>
class SharedPtr {
 public:
  using element_type = T;

  constexpr SharedPtr() noexcept : ptr_(nullptr) {}
  constexpr SharedPtr(std::nullptr_t) noexcept : SharedPtr() {}

  explicit SharedPtr(T* p) : ptr_(p) {}

  template <typename Deleter>
  SharedPtr(T* p, Deleter /*d*/) : ptr_(p) {}  // TODO(anwer): own the block

  ~SharedPtr() { delete ptr_; }  // TODO(anwer): release_shared, not delete

  SharedPtr(const SharedPtr& o) noexcept : ptr_(nullptr) {  // TODO(anwer): share
    (void)o;
  }
  SharedPtr(SharedPtr&& o) noexcept : ptr_(nullptr) {  // TODO(anwer): steal
    (void)o;
  }

  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr(const SharedPtr<U>& o) noexcept : ptr_(nullptr) {  // TODO: share
    (void)o;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr(SharedPtr<U>&& o) noexcept : ptr_(nullptr) {  // TODO: steal
    (void)o;
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& o, T* p) noexcept  // TODO(anwer): alias
      : ptr_(nullptr) {
    (void)o;
    (void)p;
  }

  SharedPtr& operator=(const SharedPtr& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  SharedPtr& operator=(SharedPtr&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr& operator=(const SharedPtr<U>& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  SharedPtr& operator=(SharedPtr<U>&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }

  void reset() noexcept {
    delete ptr_;
    ptr_ = nullptr;
  }
  void reset(T* p) {  // TODO(anwer): adopt p, destroy old
    delete p;
  }
  template <typename Deleter>
  void reset(T* p, Deleter /*d*/) {  // TODO(anwer): adopt with deleter
    delete p;
  }

  void swap(SharedPtr& o) noexcept {  // TODO(anwer): exchange
    (void)o;
  }

  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  long use_count() const noexcept { return 0; }  // TODO(anwer): shared_count
  bool unique() const noexcept { return use_count() == 1; }

 private:
  T* ptr_;
};

template <typename T>
class WeakPtr {
 public:
  constexpr WeakPtr() noexcept {}

  WeakPtr(const SharedPtr<T>& o) noexcept {  // TODO(anwer): observe
    (void)o;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  WeakPtr(const SharedPtr<U>& o) noexcept {  // TODO(anwer): observe
    (void)o;
  }

  WeakPtr(const WeakPtr& o) noexcept {  // TODO(anwer): share observation
    (void)o;
  }
  WeakPtr(WeakPtr&& o) noexcept {  // TODO(anwer): steal
    (void)o;
  }
  template <typename U>
    requires(std::convertible_to<U*, T*>)
  WeakPtr(const WeakPtr<U>& o) noexcept {  // TODO(anwer)
    (void)o;
  }

  ~WeakPtr() {}

  WeakPtr& operator=(const WeakPtr& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  WeakPtr& operator=(WeakPtr&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  WeakPtr& operator=(const SharedPtr<T>& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }

  void reset() noexcept {}
  void swap(WeakPtr& o) noexcept {  // TODO(anwer): exchange
    (void)o;
  }

  long use_count() const noexcept { return 0; }  // TODO(anwer)
  bool expired() const noexcept { return true; }  // TODO(anwer)

  SharedPtr<T> lock() const noexcept {  // TODO(anwer): upgrade iff alive
    return SharedPtr<T>();
  }
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {  // TODO(anwer): single allocation
  (void)sizeof...(args);
  return SharedPtr<T>();
}

template <typename T>
void swap(SharedPtr<T>& a, SharedPtr<T>& b) noexcept {
  (void)a;
  (void)b;
}
template <typename T>
void swap(WeakPtr<T>& a, WeakPtr<T>& b) noexcept {
  (void)a;
  (void)b;
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
