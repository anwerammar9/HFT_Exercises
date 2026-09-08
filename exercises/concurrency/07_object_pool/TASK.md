# Exercise concurrency/07_object_pool (ex23) — Object Pool (Task)

## The problem (in plain words)

A fixed-capacity, **thread-safe** object pool: a **lock-free freelist** of slots
plus RAII **Handle**s. Every `acquire()` hands out a **fresh, default-
constructed** object, and returning — or simply dropping — a handle
automatically recycles the slot for the next user. This is the *thin-client /
per-request context* pattern: allocate once up front, reuse forever, never go
back to `malloc`.

## Requirements (what the tests check)

1. `acquire()` returns an owning `Handle` to a **FRESH** `T`: the slot is
   placement-new-constructed (`std::construct_at`) before being handed out, so
   even *after* reuse the object still reads as freshly default-constructed.
2. Exhausted pool → a `Handle` with `bool(h) == false`; **never throws**.
3. RAII release: destroying the handle (or calling `reset()`) destroys the
   object (`std::destroy_at`) and pushes the slot back onto the freelist —
   immediately re-acquirable.
4. Move semantics transfer the `{pool, obj}` pair and leave the source handle
   empty — so each slot is recycled **exactly once**. Copying a handle is
   deleted.
5. `Handle::release()` **detaches** without recycling: the slot is gone for
   good (an acquire-after-release test sees the now-shorter pool). The caller
   owns the object.
6. `capacity()` is fixed.
7. Every handle must be returned before the pool dies; the destructor
   **asserts no live objects** (`live_ == 0`).
8. Concurrent acquire/release never hands the same slot to two threads
   (`tsan;stress`).

## Public API

```cpp
template <typename T>
class ObjectPool {
  explicit ObjectPool(std::size_t capacity);
  ~ObjectPool();  // asserts no live handles
  class Handle {  // move-only RAII owner
    explicit operator bool() const;
    T* get() const;  T* operator->() const;  T& operator*() const;
    T* release();    // detach from pool, caller owns the object
    void reset();    // return to pool immediately
  };
  Handle acquire();
  std::size_t capacity() const noexcept;
};
```

The stub lives in `include/object_pool.h` (a template) — implement inline.

## How to think about it (suggested design)

- A contiguous arena of `capacity_` `Slot`s: aligned `T` storage (as the **first
  member**, so a `T*` reinterprets straight to its `Slot*`) plus a freelist
  `next` pointer.
- **Lock-free freelist** through `head_` (atomic `Slot*`): acquire does a
  CAS-pop; release does a CAS-push; `std::this_thread::yield()`/backoff on
  contention. No locks on the hot path — same shape as Exercises 11/22.
- `Handle` is the move-only owner: its `reset()` is the single recycle point;
  move-assign calls `reset()` on the *destination* first. `live_` is a plain
  counter updated for the ctor/dtor assertion.
- **Compare with Exercise 08:** 08 runs ctor/dtor on raw `T*` and is
  single-threaded; this pool is synchronized and hands out a self-recycling
  owner. Call out the difference in a comment.

## Make it harder (optional — not covered by the tests)

- **`acquire(Args&&...)`:** construct the object with *arguments* instead of
  default-constructing (pooled request contexts with a `request_id`).
- **Batch acquire:** `acquire_n(k)` grabbing several slots with one CAS burst,
  plus a `release_all()` that recycles in bulk.
- **Peak-usage metrics:** track `peak_live` and a `stats()` (allocs, frees,
  live) so a leak of handles is visible before the assert fires.
- **Poisoning on recycle:** fill freed `T` storage with a pattern so
  use-after-recycle is debuggable.
- **Hazard-pointer-free IRQ-safe path:** document — or build — the variant
  where `release` is only ever called from one thread (freelist degrades to a
  single atomic head claim).

## Files

- Stub: `include/object_pool.h` (template — implement inline)
- Tests: `test/test_object_pool.cpp`
- Reference: `SOLUTION.md`