# Exercise memory/05_borrow_guards (ex46) — Runtime Borrow Guards (Reference Solution)

**What you implement:** a `RefCell`-style runtime borrow box (`BorrowBox`)
with move-only shared (`BorrowRef`) and exclusive (`BorrowMut`) RAII guards:
many readers XOR one writer, enforced by counters, released by destruction.

**Approach**
- Box members: `T value_`, `size_t readers_`, `bool writer_`. Guards hold the
  payload pointer plus an owning `owner_` back-pointer; destruction (and
  move-assign's release of the old guard) calls back into `release_shared()`
  / `release_mut()`.
- `try_borrow()` refuses iff `writer_`, else bumps first and wraps (the guard
  always owns a counted borrow). `try_borrow_mut()` refuses unless fully
  free, else sets the flag first for the same reason.
- Moves steal both pointers and null the source; the moved-from guard's
  destructor then no-ops, so no count can leak or double-release. Self-move
  is guarded. Moves never touch the count: the borrow outlives the transfer.
- Single-threaded by contract — plain counters, no atomics.

## Reference API + implementation — `include/borrow_guards.h`

The classes are templates, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE46_BORROW_GUARDS_H_
#define EXERCISE46_BORROW_GUARDS_H_

#include <optional>
#include <type_traits>
#include <utility>

template <typename T>
class BorrowBox;

// Shared (read-only) borrow guard: RAII release on destruction. Move-only:
// at most one guard object owns each active borrow... (copies would double
// release; share by const-ref instead).
template <typename T>
class BorrowRef {
 public:
  BorrowRef() noexcept : ptr_(nullptr), owner_(nullptr) {}
  BorrowRef(const BorrowRef&) = delete;
  BorrowRef& operator=(const BorrowRef&) = delete;
  BorrowRef(BorrowRef&& o) noexcept : ptr_(o.ptr_), owner_(o.owner_) {
    o.ptr_ = nullptr;
    o.owner_ = nullptr;
  }
  BorrowRef& operator=(BorrowRef&& o) noexcept {
    if (this != &o) {
      release();
      ptr_ = o.ptr_;
      owner_ = o.owner_;
      o.ptr_ = nullptr;
      o.owner_ = nullptr;
    }
    return *this;
  }
  ~BorrowRef() { release(); }

  const T& operator*() const noexcept { return *ptr_; }
  const T* operator->() const noexcept { return ptr_; }
  const T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class BorrowBox<T>;
  BorrowRef(const T* p, BorrowBox<T>* owner) noexcept
      : ptr_(p), owner_(owner) {}
  void release() noexcept;

  const T* ptr_;
  BorrowBox<T>* owner_;
};

// Exclusive (mutable) borrow guard: at most one exists, and none while any
// BorrowRef is alive. Move-only, RAII release.
template <typename T>
class BorrowMut {
 public:
  BorrowMut() noexcept : ptr_(nullptr), owner_(nullptr) {}
  BorrowMut(const BorrowMut&) = delete;
  BorrowMut& operator=(const BorrowMut&) = delete;
  BorrowMut(BorrowMut&& o) noexcept : ptr_(o.ptr_), owner_(o.owner_) {
    o.ptr_ = nullptr;
    o.owner_ = nullptr;
  }
  BorrowMut& operator=(BorrowMut&& o) noexcept {
    if (this != &o) {
      release();
      ptr_ = o.ptr_;
      owner_ = o.owner_;
      o.ptr_ = nullptr;
      o.owner_ = nullptr;
    }
    return *this;
  }
  ~BorrowMut() { release(); }

  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class BorrowBox<T>;
  BorrowMut(T* p, BorrowBox<T>* owner) noexcept : ptr_(p), owner_(owner) {}
  void release() noexcept;

  T* ptr_;
  BorrowBox<T>* owner_;
};

// Runtime borrow checker (a std:: RefCell-style box): many shared borrows OR
// exactly one mutable borrow, enforced at runtime with RAII guards.
//
// Contract:
//   - try_borrow() succeeds iff no mutable borrow is active.
//   - try_borrow_mut() succeeds iff no borrow of any kind is active.
//   - Guards release on destruction (or move); moved-from guards are empty.
//   - Introspection: readers() / is_writing() expose the current state.
//   - Single-threaded by contract (like RefCell: no atomics, no Sync).
template <typename T>
class BorrowBox {
 public:
  template <typename... Args>
  explicit BorrowBox(Args&&... args) : value_(std::forward<Args>(args)...) {}

  BorrowBox(const BorrowBox&) = delete;
  BorrowBox& operator=(const BorrowBox&) = delete;

  std::optional<BorrowRef<T>> try_borrow() {
    if (writer_) return std::nullopt;
    ++readers_;
    return BorrowRef<T>(&value_, this);
  }
  std::optional<BorrowMut<T>> try_borrow_mut() {
    if (writer_ || readers_ != 0) return std::nullopt;
    writer_ = true;
    return BorrowMut<T>(&value_, this);
  }

  std::size_t readers() const noexcept { return readers_; }
  bool is_writing() const noexcept { return writer_; }

 private:
  friend class BorrowRef<T>;
  friend class BorrowMut<T>;
  void release_shared() noexcept { --readers_; }
  void release_mut() noexcept { writer_ = false; }

  T value_;
  std::size_t readers_ = 0;
  bool writer_ = false;
};

template <typename T>
void BorrowRef<T>::release() noexcept {
  if (owner_ != nullptr) {
    owner_->release_shared();
    owner_ = nullptr;
    ptr_ = nullptr;
  }
}

template <typename T>
void BorrowMut<T>::release() noexcept {
  if (owner_ != nullptr) {
    owner_->release_mut();
    owner_ = nullptr;
    ptr_ = nullptr;
  }
}

static_assert(!std::is_copy_constructible_v<BorrowRef<int>>);
static_assert(!std::is_copy_constructible_v<BorrowMut<int>>);
static_assert(std::is_move_constructible_v<BorrowRef<int>>);
static_assert(std::is_move_constructible_v<BorrowMut<int>>);

#endif  // EXERCISE46_BORROW_GUARDS_H_
```
