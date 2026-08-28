# Exercise 23 — Object Pool (Task)

## Problem
A fixed-capacity, thread-safe object pool: a lock-free freelist of slots plus
RAII `Handle`s. Every acquire hands out a **fresh, default-constructed**
object, and returning (or dropping) a handle automatically recycles the slot.

## Requirements (what the tests check)
1. `acquire()` returns an owning `Handle` to a FRESH `T`: the slot is
   placement-new-constructed (`std::construct_at`) before being handed out, so
   after reuse the object still reads as freshly default-constructed.
2. Exhausted pool → `Handle` with `bool(h) == false`; **never throws**.
3. RAII release: destroying the handle (or calling `reset()`) destroys the
   object (`std::destroy_at`) and pushes the slot back onto the freelist —
   immediately re-acquirable.
4. Move semantics transfer the `{pool, obj}` pair and leave the source handle
   empty — **exactly one recycle**. Copy is deleted.
5. `Handle::release()` **detaches** without recycling: the slot is gone for
   good (an acquire-after-release test sees the exhausted pool). Caller owns
   the object.
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

## Design notes
Slot arena: `capacity_` contiguous `Slot`s (aligned `T` storage as FIRST
member so a `T*` reinterprets straight to its `Slot*` + freelist `next`
pointer). Lock-free freelist through `head_` (atomic `Slot*`): CAS-pop on
acquire, CAS-push on release; `yield()`/backoff on contention. No locks on the
hot path.

## Files
- Stub: `include/object_pool.h` (template — implement inline)
- Tests: `test/test_object_pool.cpp`
- Reference: `SOLUTION.md`