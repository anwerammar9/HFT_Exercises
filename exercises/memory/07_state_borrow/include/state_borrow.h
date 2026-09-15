#ifndef EXERCISE48_STATE_BORROW_H_
#define EXERCISE48_STATE_BORROW_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

// TODO(anwer): implement typestate borrowing (see SOLUTION.md).
//
// Contract:
//   - SBox (free): borrow() & always succeeds; borrow_mut() && consumes the
//     box into (LockedBox, SMut), throwing first if readers are live.
//   - LockedBox: release(SMut) && is the only way back to SBox.
//   - SRef is copyable + const-only; SMut is move-only.
//
// Suggested shape (see SOLUTION.md):
//   - SBox { unique_ptr<T> value_ (stable address for SMut!), readers_ }.
//   - SRef { const T*, owner* } with bump-on-copy/drop-on-dtor;
//     SMut { T* } bare; LockedBox { SBox } by value.
template <typename T>
class SBox;
template <typename T>
class LockedBox;

template <typename T>
class SRef {
 public:
  SRef() noexcept : ptr_(nullptr), owner_(nullptr) {}
  SRef(const SRef& o) noexcept : ptr_(nullptr), owner_(nullptr) {  // TODO
    (void)o;
  }
  SRef& operator=(const SRef& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  SRef(SRef&& o) noexcept : ptr_(nullptr), owner_(nullptr) {  // TODO
    (void)o;
  }
  SRef& operator=(SRef&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~SRef() {}  // TODO(anwer): drop the reader count

  const T& operator*() const noexcept { return *ptr_; }
  const T* operator->() const noexcept { return ptr_; }
  const T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class SBox<T>;
  SRef(const T* /*p*/, SBox<T>* /*owner*/) noexcept
      : ptr_(nullptr), owner_(nullptr) {}

  const T* ptr_;
  SBox<T>* owner_;
};

template <typename T>
class SMut {
 public:
  SMut() noexcept : ptr_(nullptr) {}
  SMut(const SMut&) = delete;
  SMut& operator=(const SMut&) = delete;
  SMut(SMut&& o) noexcept : ptr_(nullptr) {  // TODO(anwer): steal
    (void)o;
  }
  SMut& operator=(SMut&& o) noexcept {  // TODO(anwer): steal
    (void)o;
    return *this;
  }
  ~SMut() = default;

  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class SBox<T>;
  explicit SMut(T* /*p*/) noexcept : ptr_(nullptr) {}

  T* ptr_;
};

template <typename T>
class SBox {
 public:
  template <typename... Args>
  explicit SBox(Args&&... args)
      : value_(std::make_unique<T>(std::forward<Args>(args)...)) {}

  SBox(const SBox&) = delete;
  SBox& operator=(const SBox&) = delete;
  SBox(SBox&&) noexcept = default;
  SBox& operator=(SBox&&) noexcept = default;

  std::optional<SRef<T>> borrow() & {  // TODO(anwer)
    return std::nullopt;
  }

  std::pair<LockedBox<T>, SMut<T>> borrow_mut() && {  // TODO(anwer)
    throw std::logic_error("not implemented");
  }

  std::size_t readers() const noexcept { return readers_; }

 private:
  std::unique_ptr<T> value_;
  std::size_t readers_ = 0;
};

template <typename T>
class LockedBox {
 public:
  LockedBox(const LockedBox&) = delete;
  LockedBox& operator=(const LockedBox&) = delete;
  LockedBox(LockedBox&&) noexcept = default;
  LockedBox& operator=(LockedBox&&) noexcept = default;

  SBox<T> release(SMut<T> /*m*/) && {  // TODO(anwer): return the box
    throw std::logic_error("not implemented");
  }

 private:
  SBox<T> box_;
};

#endif  // EXERCISE48_STATE_BORROW_H_
