# Exercise memory/07_state_borrow (ex48) — Typestate Borrowing (Task)

## The problem (in plain words)

ex46 checks shared-XOR-mutable at runtime; ex47 tracks it with manual tokens.
This exercise moves half the rule into the **type system** (typestate): the
owner exists in two states — `SBox` (free, borrowable) and `LockedBox`
(exclusive loan outstanding). The exclusive path *consumes* the box
(`borrow_mut() &&`), so "borrow while mutably borrowed" is inexpressible in
code, not merely a runtime failure. Shared handles stay copyable with
const-only access; the exclusive handle is move-only. The one dynamic remnant
— how many shared handles are live when `borrow_mut` is attempted — is still
checked at runtime.

## Requirements (what the tests check)

1. A fresh `SBox` is free (`readers() == 0`).
2. Shared borrows always succeed while the box is alive (no writer can exist
   alongside it — that is the static guarantee, observed at runtime).
3. Shared handles are const-only: `*ref` is `const T&` (`static_assert`),
   and `SRef` is copyable while `SMut` is move-only (`static_assert`s).
4. `borrow_mut()` consumes the box (`std::move(box).borrow_mut()`) and yields
   a `(LockedBox, SMut)` pair; mutation through `SMut` persists.
5. `borrow_mut()` on a box with live readers throws `std::logic_error`
   (the box is not consumed — the throw precedes the move).
6. `release()` consumes lock + handle together and returns a borrowable
   `SBox`; reader counts are exact across copy/drop nesting.

## Public API

```cpp
template <typename T> class SRef {    // shared handle: copyable, const-only
  const T& operator*() const noexcept; const T* get() const noexcept; ...
};
template <typename T> class SMut {    // exclusive handle: move-only
  T& operator*() const noexcept; T* get() const noexcept; ...
};
template <typename T> class SBox {    // free-state owner
  template <typename... Args> explicit SBox(Args&&...);
  SBox(const SBox&) = delete;
  std::optional<SRef<T>> borrow() &;                        // always succeeds
  std::pair<LockedBox<T>, SMut<T>> borrow_mut() &&;         // consumes; throws if readers live
  std::size_t readers() const noexcept;
};
template <typename T> class LockedBox {  // locked-state owner
  LockedBox(const LockedBox&) = delete;
  SBox<T> release(SMut<T>) &&;          // the only way back
};
```

The stub lives in `include/state_borrow.h` (templates — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- `SBox` holds `T value_ + size_t readers_`. `SRef` holds `const T* + SBox*`;
  copies bump, dtor drops (mirror ex46's guard mechanics for the shared side).
- `borrow_mut() &&`: check `readers_ == 0` (throw first — before moving!),
  then move `*this` into a `LockedBox` and hand out an `SMut` pointing at the
  moved value. The `&&` qualifier is the whole trick: callers must
  `std::move`, so the old box cannot be used afterwards.
- `LockedBox` holds the `SBox` by value; `release(SMut) &&` drops the token
  and moves the box out. `SMut` needs no owner pointer — but document the
  cost: dropping it without `release()` strands the box.
- Friendship: `SBox ↔ SRef` (count bumps), `SBox → LockedBox` (private ctor
  + `value_ptr()`), `SBox → SMut` (private ctor).

## Make it harder (optional — not covered by the tests)

- **Negative compile tests:** add `static_assert(!std::is_invocable_v<...>)`
  proofs that `borrow_mut` cannot be called on an lvalue (the `&&`
  qualifier as a compile-time firewall).
- **Downgrade:** `LockedBox::downgrade(SMut) &&` → `(SBox, SRef)`: convert an
  exclusive loan into a shared one without fully releasing.
- **Typestate across threads:** a `SendBox` whose `borrow_mut() &&` hands the
  pair to another thread (exclusive ownership transfer as the thread-safety
  argument — the `std::mutex` alternative).

## Files

- Stub: `include/state_borrow.h` (templates — implement inline)
- Tests: `test/test_state_borrow.cpp`
- Reference: `SOLUTION.md`
