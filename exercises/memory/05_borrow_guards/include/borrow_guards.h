#ifndef EXERCISE46_BORROW_GUARDS_H_
#define EXERCISE46_BORROW_GUARDS_H_

#include <optional>
#include <type_traits>
#include <utility>

// TODO(anwer): implement runtime shared-XOR-mutable guards (see SOLUTION.md).
//
// Contract:
//   - try_borrow() succeeds iff no mutable borrow is active (many may coexist).
//   - try_borrow_mut() succeeds iff fully free.
//   - Guards are move-only and release on destruction; moves never change counts.
//
// Suggested shape (see SOLUTION.md):
//   - BorrowBox { T value_; size_t readers_; bool writer_; }.
//   - Guards hold (payload pointer, owner_); dtor/move-release calls back
//     into release_shared()/release_mut(); moves steal + null the source.
template <typename T>
class BorrowBox;

template <typename T>
class BorrowRef {
 public:
  BorrowRef() noexcept : ptr_(nullptr), owner_(nullptr) {}
  BorrowRef(const BorrowRef&) = delete;
  BorrowRef& operator=(const BorrowRef&) = delete;
  BorrowRef(BorrowRef&& o) noexcept : ptr_(nullptr), owner_(nullptr) {  // TODO
    (void)o;
  }
  BorrowRef& operator=(BorrowRef&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~BorrowRef() {}  // TODO(anwer): release the shared borrow

  const T& operator*() const noexcept { return *ptr_; }
  const T* operator->() const noexcept { return ptr_; }
  const T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class BorrowBox<T>;
  BorrowRef(const T* p, BorrowBox<T>* owner) noexcept
      : ptr_(nullptr), owner_(nullptr) {  // TODO(anwer): hold the borrow
    (void)p;
    (void)owner;
  }

  const T* ptr_;
  BorrowBox<T>* owner_;
};

template <typename T>
class BorrowMut {
 public:
  BorrowMut() noexcept : ptr_(nullptr), owner_(nullptr) {}
  BorrowMut(const BorrowMut&) = delete;
  BorrowMut& operator=(const BorrowMut&) = delete;
  BorrowMut(BorrowMut&& o) noexcept : ptr_(nullptr), owner_(nullptr) {  // TODO
    (void)o;
  }
  BorrowMut& operator=(BorrowMut&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~BorrowMut() {}  // TODO(anwer): release the exclusive borrow

  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  T* get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  friend class BorrowBox<T>;
  BorrowMut(T* p, BorrowBox<T>* owner) noexcept
      : ptr_(nullptr), owner_(nullptr) {  // TODO(anwer): hold the borrow
    (void)p;
    (void)owner;
  }

  T* ptr_;
  BorrowBox<T>* owner_;
};

template <typename T>
class BorrowBox {
 public:
  template <typename... Args>
  explicit BorrowBox(Args&&... args) : value_(std::forward<Args>(args)...) {}

  BorrowBox(const BorrowBox&) = delete;
  BorrowBox& operator=(const BorrowBox&) = delete;

  std::optional<BorrowRef<T>> try_borrow() {  // TODO(anwer)
    return std::nullopt;
  }
  std::optional<BorrowMut<T>> try_borrow_mut() {  // TODO(anwer)
    return std::nullopt;
  }

  std::size_t readers() const noexcept { return readers_; }
  bool is_writing() const noexcept { return writer_; }

 private:
  T value_;
  std::size_t readers_ = 0;
  bool writer_ = false;
};

static_assert(!std::is_copy_constructible_v<BorrowRef<int>>);
static_assert(!std::is_copy_constructible_v<BorrowMut<int>>);
static_assert(std::is_move_constructible_v<BorrowRef<int>>);
static_assert(std::is_move_constructible_v<BorrowMut<int>>);

#endif  // EXERCISE46_BORROW_GUARDS_H_
