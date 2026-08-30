# Exercise 15 — From-Scratch Dynamic Vector (Task)

## The problem (in plain words)

Re-implement `std::vector`: contiguous storage, geometric (2×) growth, correct
value semantics, and — the part people actually get wrong — **explicit, manual
object lifetime**. Storage comes from raw `::operator new`; elements are born
with placement `new` and die with an explicit destructor call. When the buffer
grows, live objects must be **moved** (if that is possible without throwing) so
nothing is needlessly copied.

## Requirements (what the tests check)

1. `push_back`/`emplace_back` append; `size()` grows by one.
2. Growth is **geometric (2×)**: after pushing N elements,
   `capacity() ∈ [N, 2·N)`.
3. **Contiguity:** iterators are raw pointers; `begin()`/`end()` iterate exactly
   `data() + i` — no hidden nodes, no sentinels.
4. Reallocation **moves** elements when `T` is nothrow-move-constructible (a
   `Tracked` type proves `copies == 0` on growth) and **copies** otherwise —
   with strong exception safety in both cases.
5. `at(i)` throws `std::out_of_range` past `size()`; `operator[]` is unchecked.
6. `pop_back`/`clear` destroy elements; `clear()` **retains storage**;
   `shrink_to_fit()` reallocates to exactly `size()`; `reserve(n)` is a no-op
   when `n <= capacity()`, otherwise grows to `capacity >= n`.
7. Copy ctor + copy assign deep-copy; move ctor + move assign steal and leave
   the source **empty**; no aliasing. (`std::unique_ptr`-element containers must
   fully instantiate and work.)
8. Storage from `::operator new`; every element placement-new'd and explicitly
   destroyed — constructor/destructor counts balance.

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

The stub lives in `include/dynamic_vector.h` (a template) — implement inline.

## How to think about it (suggested design)

- Three members: `T* data_`, `size_`, `capacity_`; storage from
  `::operator new(size * sizeof(T))`.
- **Grow-and-append** (the heart of the exercise): allocate a `2×` buffer,
  *move* or *copy* each live element into it (`noexcept` move check on the raw
  type, nothing exotic), destroy the old elements, free the old buffer, then
  construct the new element. If a move/copy throws, destroy what you built and
  rethrow — the vector must still hold the *old* contents.
- **Move ctor:** steal `data_` and set the source to `nullptr/0/0` (look at the
  already-written stub move-assign for the pattern).
- Every element construction is an explicit `new (ptr) T(...)` and every
  destruction an explicit `ptr->~T()` — the tests will count them.

## Make it harder (optional — not covered by the tests)

- **`insert`/`erase` at an index:** shift elements (move-shuffle) and keep the
  strong exception guarantee.
- **`resize(n)` / `assign`:** grow-or-shrink to a count, default-constructing
  (or value-copying) the new tail.
- **`const_iterator` and `operator==`:** value-compare two vectors via a simple
  loop (this is where people discover element type requirements).
- **`emplace` with a `K`-arity check:** prove `emplace_back(3 args)` only ever
  calls a 3-arg constructor.
- **Stress both growth policies:** run the `Tracked` test at 1M elements and
  assert the move/copy counts stay 0/expected.

## Files

- Stub: `include/dynamic_vector.h` (template — implement inline)
- Tests: `test/test_dynamic_vector.cpp`
- Reference: `SOLUTION.md`