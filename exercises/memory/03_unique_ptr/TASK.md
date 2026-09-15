# Exercise memory/03_unique_ptr (ex44) — Unique Pointer (Task)

## The problem (in plain words)

Raw `new`/`delete` ownership is the classic HFT bug factory: who deletes this
order object, on which path, exactly once? `std::unique_ptr` answers with
**exclusive ownership + move semantics** — exactly one owner, transfers are
explicit (`std::move`), destruction is automatic through a configurable
deleter. Reimplement it from scratch to prove you understand every rule:
move-only lifetime, `release` vs `reset`, custom deleters, array
specialization, and the Derived → Base converting move.

## Requirements (what the tests check)

1. Default / `nullptr` construction is empty (`get() == nullptr`,
   `operator bool` is false, compares equal to `nullptr`).
2. The explicit raw-pointer constructor takes ownership; the destructor
   destroys the object exactly once (tracked type: `alive` returns to 0).
3. `operator*` / `operator->` dereference the owned object.
4. Move construction transfers ownership and empties the source (no double
   ownership, no double delete).
5. Move assignment destroys the old object, transfers the new one, empties
   the source; **self-move-assign is a safe no-op**.
6. `release()` returns the raw pointer and empties the holder **without**
   destroying (caller owns it now).
7. `reset(p)` destroys the old object and owns `p`; `reset()` / `reset(nullptr)`
   just destroys and empties.
8. `swap` (member + free function) exchanges ownership.
9. A custom deleter (`CountingDeleter`, `FreeDeleter` over `malloc` memory) is
   invoked instead of plain `delete` — including a `malloc`/`free` pair that
   must stay clean under ASan.
10. Derived → Base converting move works with virtual dispatch intact.
11. The `T[]` specialization manages arrays: `operator[]`, `delete[]` (all
    element dtors run).
12. `MakeUnique<T>(args...)` forwards constructor args; `MakeUniqueArray<T>(n)`
    value-initializes; `MakeUniqueArray<T>(n, value)` fills.
13. Non-copyable: `static_assert`s prove copy ctor/assign are deleted while
    moves exist.

## Public API

```cpp
template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
  UniquePtr();                              // empty
  UniquePtr(std::nullptr_t);
  explicit UniquePtr(T* p);
  UniquePtr(T* p, const Deleter& d);
  UniquePtr(T* p, Deleter&& d);
  ~UniquePtr();                             // destroys via deleter
  UniquePtr(const UniquePtr&) = delete;     // move-only
  UniquePtr& operator=(const UniquePtr&) = delete;
  UniquePtr(UniquePtr&& o) noexcept;        // source emptied
  UniquePtr& operator=(UniquePtr&& o) noexcept;
  template <typename U, typename E> UniquePtr(UniquePtr<U, E>&& o);  // converting
  template <typename U, typename E> UniquePtr& operator=(UniquePtr<U, E>&& o);
  T* get() const noexcept;
  Deleter& get_deleter() noexcept; const version too
  explicit operator bool() const noexcept;
  T& operator*() const noexcept;
  T* operator->() const noexcept;
  T* release() noexcept;
  void reset(T* p = nullptr) noexcept;
  void swap(UniquePtr& o) noexcept;
};
// + UniquePtr<T[], Deleter> with operator[](size_t) instead of */->
// + MakeUnique<T>(args...), MakeUniqueArray<T>(n[, value])
// + swap free function, ==/!= against nullptr
```

The stub lives in `include/unique_ptr.h` (a template — implement the
methods inline, replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- Two members: `T* ptr_` + `Deleter deleter_`. Every operation is a few lines.
- Destructor = `reset()`. Move ctor = steal pointer + move deleter, null the
  source. Move assign = `if (this != &o) { reset(o.release()); ... }` — the
  self-check is what makes self-move safe.
- `reset(p)`: stash old, store new, destroy old. `release()`: stash, store
  null, return stashed (no destroy).
- Converting move: `requires` on `convertible_to<U*, T*>` plus deleter
  constructibility; implement via the public `release()`/`get_deleter()` so no
  friendship is needed.
- Array spec: same shape, `operator[]`, and let `std::default_delete<T[]>`
  (the primary's default with `T = U[]`) do the `delete[]`.

## Make it harder (optional — not covered by the tests)

- **Reference deleters:** support `Deleter = void(&)(T*)` (function reference)
  and stateful deleters with captures; check `sizeof(UniquePtr)` stays one
  pointer for stateless deleters (empty-base optimization).
- **Three-way comparison:** add `<=>` against raw pointers and other
  `UniquePtr`s.
- **Pool deleter:** a deleter that returns the object to the ex08 pool instead
  of deleting — exclusive ownership over pooled order nodes with zero
  `malloc` in the hot path.

## Files

- Stub: `include/unique_ptr.h` (template — implement inline)
- Tests: `test/test_unique_ptr.cpp`
- Reference: `SOLUTION.md`
