# Exercise 15 — From-Scratch Dynamic Vector (Task)

## Problem
A `std::vector` clone: contiguous storage, geometric growth, proper value
semantics, and memory-safe element construction/destruction.

## Requirements (what the tests check)
1. `push_back`/`emplace_back` append; `size()` grows by one.
2. Growth is **geometric (2×)**: after pushing N elements,
   `capacity() ∈ [N, 2·N)`.
3. Contiguity: iterators are raw pointers; raw `T*` iteration == `data() + i`.
4. Reallocation **moves** when `T` is nothrow-move-constructible (a `Tracked`
   type proves `copies == 0` on growth), **copies** otherwise — strong
   exception safety both ways.
5. `at(i)` throws `std::out_of_range` past `size()`; `operator[]` never checks.
6. `pop_back`/`clear` destroy elements; `clear()` retains storage;
   `shrink_to_fit()` fits exactly; `reserve(n)` is a no-op when `n <=
   capacity()`, otherwise reallocates to `capacity >= n`.
7. Copy ctor + copy assign deep-copy; move ctor + move assign steal and leave
   the source empty; no aliasing. (`std::unique_ptr` containers fully
   instantiate.)
8. Storage from `::operator new`; elements placement-new'd and explicitly
   destroyed (ctor/dtor counts balance).

## Public API
```cpp
template <typename T>
class Vector {
  // usings (value_type, size_type, reference, pointer...)
  Vector();  Vector(size_type n);  Vector(size_type n, const T&);
  Vector(const Vector&);  Vector(Vector&&);  ~Vector();
  Vector& operator=(const Vector&);  Vector& operator=(Vector&&);
  T& operator[](size_type i);  T& at(size_type i);
  reference front()/back();  pointer data();  pointer begin()/end();
  void push_back(const T&);  void push_back(T&&);
  T& emplace_back(Args&&...);
  void pop_back();  void clear();  void reserve(size_type n);
  void shrink_to_fit();
  size_type size()/capacity();  bool empty();
};
```

## Files
- Stub: `include/dynamic_vector.h` (template — implement inline)
- Tests: `test/test_dynamic_vector.cpp`
- Reference: `SOLUTION.md`