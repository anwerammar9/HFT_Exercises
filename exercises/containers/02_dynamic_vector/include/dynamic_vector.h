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
//   - capacity() never decreases except via shrink_to_fit() (exact-fit).
//   - reserve(n): no-op when n <= capacity(), else reallocates so cap >= n.
//   - Growth policy: geometric (2x).
//   - Reallocation moves when T is nothrow-move-constructible, copies
//     otherwise (strong exception safety both ways).
//   - Storage from ::operator new; elements placement-new'd and explicitly
//     destroyed. Iterators are raw pointers into data().
//
// TODO(anwer): implement the mutators (see SOLUTION.md).
// Stub: mutators throw std::logic_error so the tests run RED.
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
  T* data_ = nullptr;
  size_type size_ = 0;
  size_type capacity_ = 0;
};

// ---------------------------------------------------------------------------
// Stub implementation (TODO: replace with the real thing from SOLUTION.md)
// ---------------------------------------------------------------------------

template <typename T>
Vector<T>::Vector(size_type) {
  throw std::logic_error("not implemented");
}

template <typename T>
Vector<T>::Vector(size_type, const T&) requires std::copy_constructible<T> {
  throw std::logic_error("not implemented");
}

template <typename T>
Vector<T>::Vector(const Vector&) requires std::copy_constructible<T> {
  throw std::logic_error("not implemented");
}

template <typename T>
Vector<T>::Vector(Vector&& o) noexcept
    : data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
  o.data_ = nullptr;
  o.size_ = o.capacity_ = 0;
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector&) requires std::copy_constructible<T> {
  throw std::logic_error("not implemented");
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
void Vector<T>::push_back(const T&) requires std::copy_constructible<T> {
  throw std::logic_error("not implemented");
}

template <typename T>
void Vector<T>::push_back(T&&) {
  throw std::logic_error("not implemented");
}

template <typename T>
template <typename... Args>
T& Vector<T>::emplace_back(Args&&...) {
  throw std::logic_error("not implemented");
}

template <typename T>
void Vector<T>::pop_back() noexcept {
  if (size_ == 0) return;
  --size_;
  data_[size_].~T();
}

template <typename T>
void Vector<T>::clear() noexcept {
  for (size_type j = 0; j < size_; ++j) data_[j].~T();
  size_ = 0;
}

template <typename T>
void Vector<T>::reserve(size_type /*n*/) {}

template <typename T>
void Vector<T>::shrink_to_fit() {}

#endif  // EXERCISE15_DYNAMIC_VECTOR_H_