# Exercise memory/07_state_borrow (ex48) — Typestate Borrowing (Reference Solution)

**What you implement:** typestate borrowing — the owner exists as `SBox`
(free) or `LockedBox` (exclusive loan outstanding), and the exclusive path
*consumes* the box (`borrow_mut() &&`), so borrowing-while-mutably-borrowed
is inexpressible rather than a runtime failure. Shared handles stay copyable
and const-only; the exclusive handle is move-only.

**Approach**
- `SBox` holds the value behind `unique_ptr<T>` — the address must stay
  stable because `SMut` handles keep pointing at it while `SBox`/`LockedBox`
  owners move around it (a raw member would dangle after the first move).
- `SRef` (copyable, `const T&` access) bumps/drops the box's reader count;
  copies share one count, self-assign is guarded.
- `SBox::borrow() &` always succeeds (no writer can exist while the box is
  alive — the static guarantee, observed at runtime). `borrow_mut() &&`
  throws *before moving* if readers are live (the one dynamic remnant),
  then moves `*this` into a `LockedBox` and points an `SMut` at the moved
  value. The `&&` qualifier forces callers to `std::move`, retiring the old
  box.
- `LockedBox::release(SMut) &&` consumes lock + token together and moves the
  box back out — the only path home. `SMut` needs no owner pointer, at a
  documented price: dropping it without `release()` strands the box.
- Friendship: `SBox ↔ SRef` (counts), `SBox → LockedBox` (private ctor,
  `value_ptr()`), `SBox → SMut` (private ctor).

## Reference API + implementation — `include/state_borrow.h`

The classes are templates, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE48_STATE_BORROW_H_
#define EXERCISE48_STATE_BORROW_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class SBox;
template <typename T>
class LockedBox;

// Shared handle: copyable, const-only access. Copies share one reader count.
template <typename T>
class SRef {
 public:
  SRef() noexcept : ptr_(nullptr), owner_(nullptr) {}
  SRef(const SRef& o) noexcept : ptr_(o.ptr_), owner_(o.owner_) {
    if (owner_ != nullptr) owner_->add_reader();
  }
  SRef& operator=(const SRef& o) noexcept {
    if (this != &o) {
      release();
      ptr_ = o.ptr_;
      owner_ = o.owner_;
      if (owner_ != nullptr) owner_->add_reader();
    }
    return *this;
  }
  SRef(SRef&& o) noexcept : ptr_(o.ptr_), owner_(o.owner_) {
    o.ptr_ = nullptr;
    o.owner_ = nullptr;
  }
  SRef& operator=(SRef&& o) noexcept {
    if (this != &o) {
      release();
      ptr_ = o.ptr_;
      owner_ = o.owner_;
      o.ptr_ = nullptr;
      o.owner_ = nullptr;
    }
    return *this;
  }
  ~SRef() { release(); }

  const T& operator*() const noexcept { return *ptr_; }
  const T* operator->() const noexcept { return ptr_; }
  const T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class SBox<T>;
  SRef(const T* p, SBox<T>* owner) noexcept : ptr_(p), owner_(owner) {}
  void release() noexcept;

  const T* ptr_;
  SBox<T>* owner_;
};

// Exclusive handle: move-only, mutable access. There is no copy, so a second
// writer is *inexpressible* — that half of the rule is compile-time.
template <typename T>
class SMut {
 public:
  SMut() noexcept : ptr_(nullptr) {}
  SMut(const SMut&) = delete;
  SMut& operator=(const SMut&) = delete;
  SMut(SMut&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }
  SMut& operator=(SMut&& o) noexcept {
    if (this != &o) {
      ptr_ = o.ptr_;
      o.ptr_ = nullptr;
    }
    return *this;
  }
  ~SMut() = default;  // NOTE: dropping an SMut without release() strands the
                      // LockedBox (see below) — the manual-discipline cost.

  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class SBox<T>;
  explicit SMut(T* p) noexcept : ptr_(p) {}

  T* ptr_;
};

// Free-state owner: shared borrows always succeed (no writer can exist while
// this object is alive — exclusivity *consumed* the owner). The exclusive
// path moves the box into a LockedBox, so borrowing-while-mut is a type
// error, not a runtime check.
template <typename T>
class SBox {
 public:
  // The value lives behind unique_ptr so its address is stable: SMut handles
  // keep pointing at it while SBox/LockedBox owners move around it.
  template <typename... Args>
  explicit SBox(Args&&... args)
      : value_(std::make_unique<T>(std::forward<Args>(args)...)) {}

  SBox(const SBox&) = delete;
  SBox& operator=(const SBox&) = delete;
  SBox(SBox&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
  SBox& operator=(SBox&&) noexcept(std::is_nothrow_move_assignable_v<T>) =
      default;

  std::optional<SRef<T>> borrow() & {
    ++readers_;
    return SRef<T>(value_.get(), this);
  }

  // Consumes the box (note &&): after this call only the LockedBox can
  // produce the owner again. Throws if shared handles are still live — the
  // one runtime remnant (counts are dynamic; exclusivity is static).
  std::pair<LockedBox<T>, SMut<T>> borrow_mut() && {
    if (readers_ != 0)
      throw std::logic_error("SBox::borrow_mut with live shared borrows");
    LockedBox<T> locked(std::move(*this));
    SMut<T> m(locked.value_ptr());
    return {std::move(locked), std::move(m)};
  }

  std::size_t readers() const noexcept { return readers_; }

 private:
  friend class SRef<T>;
  friend class LockedBox<T>;
  void add_reader() noexcept { ++readers_; }
  void release_reader() noexcept { --readers_; }

  std::unique_ptr<T> value_;
  std::size_t readers_ = 0;
};

// Locked-state owner: the ONLY way back to SBox is release(), which consumes
// both the lock and the exclusive handle together.
template <typename T>
class LockedBox {
 public:
  LockedBox(const LockedBox&) = delete;
  LockedBox& operator=(const LockedBox&) = delete;
  LockedBox(LockedBox&&) noexcept(std::is_nothrow_move_constructible_v<T>) =
      default;
  LockedBox& operator=(LockedBox&&) noexcept(
      std::is_nothrow_move_assignable_v<T>) = default;

  SBox<T> release(SMut<T> m) && {
    (void)m;  // consuming the handle is the proof the writer is gone
    return std::move(box_);
  }

 private:
  friend class SBox<T>;
  explicit LockedBox(SBox<T>&& box) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : box_(std::move(box)) {}
  T* value_ptr() noexcept { return box_.value_.get(); }

  SBox<T> box_;
};

template <typename T>
void SRef<T>::release() noexcept {
  if (owner_ != nullptr) {
    owner_->release_reader();
    owner_ = nullptr;
    ptr_ = nullptr;
  }
}

#endif  // EXERCISE48_STATE_BORROW_H_
```
