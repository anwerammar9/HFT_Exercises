# Exercise memory/04_shared_ptr (ex45) — Shared Pointer (Task)

## The problem (in plain words)

Some objects have no single owner: a config snapshot shared by every worker,
a market-data subscription held by the book builder *and* the publisher.
`std::shared_ptr` answers with an **atomic reference count in a control
block** — copies share ownership, the last owner destroys the object — plus
`std::weak_ptr`, the non-owning observer that **breaks reference cycles**
(child → parent would otherwise leak forever). Reimplement both, with the
thread-safe count and the single-allocation `make_shared` optimization.

## Requirements (what the tests check)

1. Empty `SharedPtr` / `WeakPtr`: `get() == nullptr`, `use_count() == 0`,
   `unique()` false, `expired()` true, `lock()` empty, compares to `nullptr`.
2. The explicit raw-pointer constructor takes ownership (`use_count() == 1`,
   `unique()`); the last owner destroys the object exactly once.
3. Copy shares ownership (count 2, same pointer, one object); the object dies
   exactly once when both go away.
4. Move transfers **without bumping the count** and empties the source.
5. Copy/move assignment releases the old object; self copy-assign and self
   move-assign are safe and keep ownership.
6. `reset()` releases (object dies with the last owner); `reset(p)` destroys
   the old object and adopts `p` with count 1.
7. `swap` (member + free function) exchanges ownership.
8. Custom deleters (`SharedPtr(p, deleter)`, `reset(p, deleter)`) run exactly
   once, on the last release — not before.
9. `MakeShared<T>(args...)` builds the object with count 1 in a single
   allocation (object + block together).
10. Derived → Base copy shares ownership with virtual dispatch intact.
11. The aliasing constructor shares ownership while pointing elsewhere (a
    member view keeps the whole object alive after the original resets).
12. `WeakPtr`: `lock()` upgrades while alive (count bumps, same pointer) and
    returns empty after the last `SharedPtr` dies; `expired()`/`use_count()`
    track liveness.
13. The parent/child cycle test: strong down-link + weak up-link destroys both
    nodes (proves weak breaks cycles).
14. Concurrent copies from 4 threads × 2000 keep exactly one owner (atomic
    count, no lost updates).
15. `==` / `!=` compare owned pointers (plus `nullptr` overloads).

## Public API

```cpp
class ControlBase {  // you own this design; atomics required
  void add_shared() noexcept; void release_shared() noexcept;
  void add_weak() noexcept;   void release_weak() noexcept;
  long shared_count() const noexcept;
  bool try_add_shared() noexcept;   // CAS upgrade iff alive
  virtual void dispose() noexcept = 0;
};
template <typename T> class SharedPtr {
  SharedPtr(); SharedPtr(std::nullptr_t);
  explicit SharedPtr(T* p);
  template <typename Deleter> SharedPtr(T* p, Deleter d);
  ~SharedPtr();                                    // last owner destroys
  SharedPtr(const SharedPtr&) noexcept;            // +1
  SharedPtr(SharedPtr&&) noexcept;                 // steal, source emptied
  template <typename U> SharedPtr(const SharedPtr<U>&) noexcept;  // converting
  template <typename U> SharedPtr(SharedPtr<U>&&) noexcept;
  template <typename U> SharedPtr(const SharedPtr<U>& o, T* p) noexcept;  // aliasing
  // + copy/move/converting assignment (copy-and-swap, self-safe)
  void reset() noexcept; void reset(T* p);
  template <typename Deleter> void reset(T* p, Deleter d);
  void swap(SharedPtr&) noexcept;
  T* get() const noexcept; T& operator*() const noexcept;
  T* operator->() const noexcept;
  explicit operator bool() const noexcept;
  long use_count() const noexcept; bool unique() const noexcept;
};
template <typename T> class WeakPtr {
  WeakPtr(); WeakPtr(const SharedPtr<T>&) noexcept; template version too
  WeakPtr(const WeakPtr&) noexcept; WeakPtr(WeakPtr&&) noexcept; template version too
  ~WeakPtr();
  // + assignment from WeakPtr / SharedPtr
  void reset() noexcept; void swap(WeakPtr&) noexcept;
  long use_count() const noexcept; bool expired() const noexcept;
  SharedPtr<T> lock() const noexcept;   // upgrade iff alive
};
// + MakeShared<T>(args...), swap free functions, ==/!= (incl. nullptr)
```

The stub lives in `include/shared_ptr.h` (templates — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- One `ControlBase` per object: `atomic<size_t> shared/weak`, `virtual
  dispose()`. `weak` starts at **1** — the shared owners' own stake — so the
  block is freed exactly when the last owner *and* the last weak are gone.
- `ControlWithDeleter<T, D>` stores `T*` + deleter; `ControlInline<T>` stores
  inline `alignas(T)` bytes for `MakeShared` (placement-new in, explicit dtor
  out, `delete` on factory throw).
- `release_shared`: `fetch_sub(acq_rel)`; on last, `dispose()` then drop the
  stake. `lock()` = `try_add_shared()` CAS loop (claim iff count ≠ 0).
- Copies `add_shared` in the init list order (bump before any release);
  assignment is copy/move-and-swap so self-assign is trivially safe.
- Concurrent copies of *distinct* `SharedPtr` objects are safe; touching the
  *same* `SharedPtr` object from two threads is a data race (same rule as std).

## Make it harder (optional — not covered by the tests)

- **`owner_before` ordering:** add strict-weak ordering across related
  `SharedPtr`/`WeakPtr` (control-block address) for use as map keys.
- **Array support:** a `SharedPtr<T[]>` specialization with `operator[]`
  (needs `default_delete<T[]>` + size tracking for bounds-checked access).
- **`allocate_shared`:** a control block that allocates from the ex07 arena
  instead of `new` — shared ownership over scratch memory with bulk reclaim.

## Files

- Stub: `include/shared_ptr.h` (templates — implement inline)
- Tests: `test/test_shared_ptr.cpp`
- Reference: `SOLUTION.md`
