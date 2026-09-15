# Exercise memory/05_borrow_guards (ex46) — Runtime Borrow Guards (Task)

## The problem (in plain words)

An order book has many readers (pricing, risk, fan-out) but only one thread
may mutate it at a time. Rust enforces *shared-XOR-mutable* at compile time;
in C++ the same discipline can be enforced **at runtime** à la `RefCell`: a
box hands out shared (`BorrowRef`) or exclusive (`BorrowMut`) guards, refuses
conflicting requests, and releases automatically when guards drop. Implement
the box, the two guard types, and the exact conflict rules.

## Requirements (what the tests check)

1. A fresh box is free: `readers() == 0`, `is_writing()` false.
2. `try_borrow()` succeeds iff no mutable borrow is active; any number of
   shared borrows may coexist (`readers()` counts them).
3. `try_borrow_mut()` succeeds iff the box is fully free — no readers, no
   writer.
4. While a writer is active, both `try_borrow()` and `try_borrow_mut()` fail.
5. A live reader makes `try_borrow_mut()` fail; once all readers drop, the
   mutable borrow succeeds.
6. Mutation through `BorrowMut` is visible to later readers.
7. Guards release on destruction (scope exit restores the free state).
8. Guards are move-only (`static_assert`s); moving a guard transfers the
   borrow without changing the count — the moved-from guard is empty and a
   second release never happens.
9. A 10-round read/read/write interleave keeps exact counts throughout.

## Public API

```cpp
template <typename T> class BorrowRef {   // shared guard, move-only
  const T& operator*() const noexcept;
  const T* operator->() const noexcept;
  const T* get() const noexcept;
  explicit operator bool() const noexcept;
};
template <typename T> class BorrowMut {   // exclusive guard, move-only
  T& operator*() const noexcept;
  T* operator->() const noexcept;
  T* get() const noexcept;
  explicit operator bool() const noexcept;
};
template <typename T> class BorrowBox {
  template <typename... Args> explicit BorrowBox(Args&&... args);
  BorrowBox(const BorrowBox&) = delete;             // the box itself is fixed
  std::optional<BorrowRef<T>> try_borrow();         // nullopt if writing
  std::optional<BorrowMut<T>> try_borrow_mut();     // nullopt unless free
  std::size_t readers() const noexcept;
  bool is_writing() const noexcept;
};
```

The stub lives in `include/borrow_guards.h` (templates — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- Three members on the box: `T value_`, `size_t readers_`, `bool writer_`.
- Guards hold `owner_` + pointer; the destructor (and move-assign's release
  of the old guard) calls back into `release_shared()` / `release_mut()`.
- Move = steal pointers, null the source; the moved-from guard's destructor
  then no-ops. Self-move-assign guards with `this != &o`.
- `try_borrow` bumps *before* wrapping (so the guard always owns a counted
  borrow); `try_borrow_mut` sets the flag first for the same reason.
- Single-threaded by contract — plain counters, no atomics (that is what
  makes this the cheap single-threaded discipline; the threaded version is
  `RwLock`, not this exercise).

## Make it harder (optional — not covered by the tests)

- **Panic-free `borrow()`:** add `borrow()` / `borrow_mut()` that throw
  `std::logic_error` on conflict (the `RefCell::borrow` API) and keep
  `try_` as the non-throwing path.
- **Mapped guards:** `BorrowRef::map(fn)` producing a guard over a member
  (e.g. borrow the whole book, hand out a guard to one level) without
  releasing the box.
- **Upgrade attempt:** `try_upgrade(BorrowRef<T>&&)` → `optional<BorrowMut<T>>`
  that succeeds only when this is the *last* reader (the classic deadlock
  footgun — document why unconditional upgrade is unsound).

## Files

- Stub: `include/borrow_guards.h` (templates — implement inline)
- Tests: `test/test_borrow_guards.cpp`
- Reference: `SOLUTION.md`
