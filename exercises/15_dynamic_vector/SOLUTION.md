# Exercise 15 — From-Scratch Dynamic Vector (Reference Solution)

**What you implement:** a `std::vector` clone with contiguous storage, geometric
growth, correct value semantics, and memory-safe construction/destruction.

**Approach**
- `grow_to(new_cap)`: `::operator new(new_cap * sizeof(T))`, move-construct the
  existing elements when `T` is nothrow-move-constructible and copy-construct
  otherwise (both give strong exception safety: a pop + operator delete on
  failure rethrows); destroy the old elements after the new block is built.
- Push paths behind `if (size_ == capacity_) grow_to(capacity_ == 0 ? 1 : 2x)`.
- `Vector(n)` / `Vector(n, v)`: grow, then value-/copy-construct n elements.
- Copy ctor/move ctor/assign leave no aliasing: move steals `data_` and nils
  the source; copy is `tmp`-and-move for strong safety.
- `at()` throws `std::out_of_range` when `i >= size_`; `operator[]` never
  checks. `pop_back` marches `size_` down and destroys; `clear` destroys all
  and zeroes size (storage retained); `shrink_to_fit` grows-to-size;
  `reserve` grows when `n > capacity_` (else no-op).
- `requires std::copy_constructible<T>` guards the copy paths so
  `Vector<std::unique_ptr<int>>` still fully instantiates.
- Destructor destroys live elements then frees storage.

## Reference API — `include/dynamic_vector.h`
#ifndef EXERCISE15_DYNAMIC_VECTOR_H_
#define EXERCISE15_DYNAMIC_VECTOR_H_

#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

// From-scratch std::vector: contiguous storage, geometric growth, proper
// value semantics, memory-safe element construction/destruction.
//
// Contract:
//   - push_back/emplace_back append; size() grows by one.
//   - capacity() never decreases except via shrink_to_fit(), which leaves
//     capacity() == size() (exact-fit) while preserving elements.
//   - reserve(n): no-op when n <= capacity(), otherwise reallocates so
//     capacity() >= n. Elements survive a reallocation with stable values.
//   - Growth policy: geometric (2x); capacity() stays within [size, 2*size]
//     after append-only pushes.
//   - Reallocation moves elements when T is nothrow-move-constructible and
//     copies otherwise (strong exception safety either way).
//   - pop_back removes (destroys) the last element; tests never call it empty.
//   - Storage comes from ::operator new; elements are placement-new'd and
//     explicitly destroyed. Iterators are raw pointers into data().

template <typename T>
class Vector {
 public:
  using value_type = T;
  using size_type = std::size_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;

  Vector() = default;
  explicit Vector(size_type n);
  Vector(size_type n, const T& value) requires std::copy_constructible<T>;
  Vector(const Vector&) requires std::copy_constructible<T>;
  Vector(Vector&&) noexcept;
  Vector& operator=(const Vector&) requires std::copy_constructible<T>;
  Vector& operator=(Vector&&) noexcept;
  ~Vector();

  reference operator[](size_type i) noexcept { return data_[i]; }
  const_reference operator[](size_type i) const noexcept { return data_[i]; }
  reference at(size_type i);
  const_reference at(size_type i) const;
  reference front() noexcept { return data_[0]; }
  const_reference front() const noexcept { return data_[0]; }
  reference back() noexcept { return data_[size_ - 1]; }
  const_reference back() const noexcept { return data_[size_ - 1]; }
  pointer data() noexcept { return data_; }
  const_pointer data() const noexcept { return data_; }

  void push_back(const T& value) requires std::copy_constructible<T>;
  void push_back(T&& value);
  template <typename... Args>
  T& emplace_back(Args&&... args);
  void pop_back() noexcept;
  void clear() noexcept;
  void reserve(size_type n);
  void shrink_to_fit();

  size_type size() const noexcept { return size_; }
  size_type capacity() const noexcept { return capacity_; }
  bool empty() const noexcept { return size_ == 0; }
  pointer begin() noexcept { return data_; }
  pointer end() noexcept { return data_ + size_; }
  const_pointer begin() const noexcept { return data_; }
  const_pointer end() const noexcept { return data_ + size_; }

 private:
  void grow_to(size_type new_cap);

  T* data_ = nullptr;
  size_type size_ = 0;
  size_type capacity_ = 0;
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

template <typename T>
void Vector<T>::grow_to(size_type new_cap) {
  T* nd = nullptr;
  if (new_cap != 0) {
    nd = static_cast<T*>(::operator new(new_cap * sizeof(T)));
    size_type i = 0;
    try {
      for (; i < size_; ++i) {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
          ::new (static_cast<void*>(nd + i)) T(std::move(data_[i]));
        } else {
          ::new (static_cast<void*>(nd + i)) T(data_[i]);
        }
      }
    } catch (...) {
      while (i > 0) {
        --i;
        nd[i].~T();
      }
      ::operator delete(nd);
      throw;
    }
  }
  for (size_type j = 0; j < size_; ++j) data_[j].~T();
  ::operator delete(data_);
  data_ = nd;
  capacity_ = new_cap;
}

template <typename T>
Vector<T>::Vector(size_type n) {
  grow_to(n);
  for (size_type i = 0; i < n; ++i) {
    ::new (static_cast<void*>(data_ + i)) T();
  }
  size_ = n;
}

template <typename T>
Vector<T>::Vector(size_type n, const T& value) requires std::copy_constructible<T> {
  grow_to(n);
  for (size_type i = 0; i < n; ++i) {
    ::new (static_cast<void*>(data_ + i)) T(value);
  }
  size_ = n;
}

template <typename T>
Vector<T>::Vector(const Vector& o) requires std::copy_constructible<T> {
  grow_to(o.size_);
  for (size_type i = 0; i < o.size_; ++i) {
    ::new (static_cast<void*>(data_ + i)) T(o.data_[i]);
  }
  size_ = o.size_;
}

template <typename T>
Vector<T>::Vector(Vector&& o) noexcept
    : data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
  o.data_ = nullptr;
  o.size_ = o.capacity_ = 0;
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& o) requires std::copy_constructible<T> {
  if (this != &o) {
    Vector tmp(o);
    *this = std::move(tmp);
  }
  return *this;
}

template <typename T>
Vector<T>& Vector<T>::operator=(Vector&& o) noexcept {
  if (this != &o) {
    for (size_type j = 0; j < size_; ++j) data_[j].~T();
    ::operator delete(data_);
    data_ = o.data_;
    size_ = o.size_;
    capacity_ = o.capacity_;
    o.data_ = nullptr;
    o.size_ = o.capacity_ = 0;
  }
  return *this;
}

template <typename T>
Vector<T>::~Vector() {
  for (size_type j = 0; j < size_; ++j) data_[j].~T();
  ::operator delete(data_);
}

template <typename T>
T& Vector<T>::at(size_type i) {
  if (i >= size_) throw std::out_of_range("Vector::at");
  return data_[i];
}

template <typename T>
const T& Vector<T>::at(size_type i) const {
  if (i >= size_) throw std::out_of_range("Vector::at");
  return data_[i];
}

template <typename T>
void Vector<T>::push_back(const T& value) requires std::copy_constructible<T> {
  if (size_ == capacity_) grow_to(capacity_ == 0 ? 1 : capacity_ * 2);
  ::new (static_cast<void*>(data_ + size_)) T(value);
  ++size_;
}

template <typename T>
void Vector<T>::push_back(T&& value) {
  if (size_ == capacity_) grow_to(capacity_ == 0 ? 1 : capacity_ * 2);
  ::new (static_cast<void*>(data_ + size_)) T(std::move(value));
  ++size_;
}

template <typename T>
template <typename... Args>
T& Vector<T>::emplace_back(Args&&... args) {
  if (size_ == capacity_) grow_to(capacity_ == 0 ? 1 : capacity_ * 2);
  ::new (static_cast<void*>(data_ + size_)) T(std::forward<Args>(args)...);
  ++size_;
  return data_[size_ - 1];
}

template <typename T>
void Vector<T>::pop_back() noexcept {
  --size_;
  data_[size_].~T();
}

template <typename T>
void Vector<T>::clear() noexcept {
  for (size_type j = 0; j < size_; ++j) data_[j].~T();
  size_ = 0;
}

template <typename T>
void Vector<T>::reserve(size_type n) {
  if (n > capacity_) grow_to(n);
}

template <typename T>
void Vector<T>::shrink_to_fit() {
  if (size_ < capacity_) grow_to(size_);
}

#endif  // EXERCISE15_DYNAMIC_VECTOR_H_
## Reference TU — `src/dynamic_vector.cpp` (explicit instantiation)
#include "dynamic_vector.h"

#include <memory>
#include <string>

// Explicit instantiations: guaranteed-compiles for plain, noexcept-movable,
// and non-trivially-copyable element types.
template class Vector<int>;
template class Vector<double>;
template class Vector<std::string>;
template class Vector<std::unique_ptr<int>>;