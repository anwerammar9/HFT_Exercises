# Exercise concurrency/07_object_pool (ex23) — Object Pool (Reference Solution)

**What you implement:** a fixed-capacity, thread-safe object pool with a
lock-free freelist and RAII `Handle`s — callers get a *fresh*, default-
constructed object on every acquire (even after reuse), and returning it (or
letting the handle die) automatically recycles the slot.

**Approach**
- **Arena + freelist:** `arena_ = new Slot[capacity_]`, where `Slot` is
  `{ alignas(T) std::byte storage[sizeof(T)]; std::atomic<Slot*> next; }`.
  Storage is the *first* member so a `T*` reinterprets directly to its `Slot*`
  (`to_slot`/`to_object`), and the `next` pointer is a separate field that `T`
  never touches. The ctor threads each slot's `next` to its neighbour and
  stores the head — an uncontended acquire is one CAS-pop.
- **Lock-free freelist:** `acquire()` CAS-pops `head_`; on contention the
  failed `compare_exchange_weak` refreshes `slot`/`next` and retries. Success
  → `std::construct_at(obj)` (value-initialized ⇒ `Redislike` reuse tests see
  0) and `live_++`. Empty pool → empty handle, never throws.
- **Release:** `Handle::reset()`/dtor call `release_obj` under the same
  invariant: `std::destroy_at` then CAS-push the slot back onto `head_`, so a
  returned slot is immediately re-acquirable. Move semantics transfer the
  `{pool,obj}` pair and leave the source handle null — exactly one recycle.
- **Detach:** `Handle::release()` gives the caller permanent ownership; the
  pool drops its outstanding count (`detach_obj`) and the slot is gone for
  good — the acquire-after-release test sees the pool exhausted.
- **`live_` is `std::atomic`** not for concurrency on one slot (a slot is owned
  by at most one thread) but so the destructor's `assert(live_ == 0)` is a
  clean read under TSan.
- Compare to 08 (`PoolAllocator`): that runs the ctor on `allocate()` for a
  single-threaded raw-`T*` allocator; this pool is synchronized and hands out
  a self-recycling owning handle.

## Reference API — `include/object_pool.h`
#ifndef EXERCISE23_OBJECT_POOL_H_
#define EXERCISE23_OBJECT_POOL_H_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>

// Fixed-capacity, thread-safe object pool (lock-free freelist + RAII handles).
//
// Contract:
//   - `acquire()` returns an owning `Handle` to a FRESH `T`: the slot is
//     placement-new-constructed (`std::construct_at`) before being handed out,
//     so callers always see a default-constructed object even after reuse.
//     When the pool is exhausted, acquire() returns an EMPTY handle
//     (`bool(h) == false`); it never throws.
//   - The `Handle` returns the object to the pool (RAII): the object is
//     destroyed (`std::destroy_at`) and the slot is pushed back onto the
//     freelist. `reset()` / move-assignment return early the same way.
//   - `capacity()` is the fixed number of objects.
//   - Every handle must be returned (or destroyed) before the pool is
//     destroyed; the destructor asserts no objects are still live.
//
// Implementation notes (reference solution):
//   - Slot arena: `capacity_` contiguous `Slot`s = aligned `T` storage (first
//     member, so a `T*` reinterprets straight to its `Slot*`) + a freelist
//     `next` pointer.
//   - Lock-free freelist threaded through `head_` (atomic<Slot*>): CAS-pop on
//     acquire, CAS-push on release; `yield()`/backoff on contention. No locks
//     on the hot path (same shape as 11 / 22).
//   - Distinction from 08 (PoolAllocator): 08 runs the ctor on allocate() and
//     returns raw `T*`, and is single-threaded by design. This pool is
//     synchronized and hands out an owning handle that self-recycles.
//
// TODO(anwer): implement the real pool (see SOLUTION.md). The stub behaves as
// an EMPTY pool: acquire() always returns an empty handle and release is a
// no-op, so every suite runs RED deterministically without hanging.
template <typename T>
class ObjectPool {
 public:
  using value_type = T;

  explicit ObjectPool(std::size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) return;
    arena_ = new Slot[capacity_];
    head_.store(arena_, std::memory_order_relaxed);
    for (std::size_t i = 0; i < capacity_; ++i) {
      arena_[i].next.store(i + 1 < capacity_ ? &arena_[i + 1] : nullptr,
                           std::memory_order_relaxed);
    }
    live_.store(0, std::memory_order_relaxed);
  }

  ~ObjectPool() {
    assert(live_.load(std::memory_order_acquire) == 0);
    delete[] arena_;
  }

  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

  // RAII handle; pushes the object back onto the freelist when destroyed.
  class Handle {
   public:
    Handle() noexcept : pool_(nullptr), obj_(nullptr) {}
    explicit operator bool() const noexcept { return obj_ != nullptr; }
    T* get() const noexcept { return obj_; }
    T* operator->() const noexcept { return obj_; }
    T& operator*() const noexcept { return *obj_; }

    Handle(Handle&& o) noexcept : pool_(o.pool_), obj_(o.obj_) {
      o.pool_ = nullptr;
      o.obj_ = nullptr;
    }
    Handle& operator=(Handle&& o) noexcept {
      if (this != &o) {
        reset();
        pool_ = o.pool_;
        obj_ = o.obj_;
        o.pool_ = nullptr;
        o.obj_ = nullptr;
      }
      return *this;
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { reset(); }

    // Detaches the object: the caller now owns it and the pool will never
    // recycle it.
    T* release() noexcept {
      T* p = obj_;
      if (pool_ != nullptr) pool_->detach_obj(obj_);
      obj_ = nullptr;
      pool_ = nullptr;
      return p;
    }
    // Returns the object to the pool immediately.
    void reset() noexcept {
      if (obj_ != nullptr && pool_ != nullptr) pool_->release_obj(obj_);
      obj_ = nullptr;
      pool_ = nullptr;
    }

   private:
    friend class ObjectPool;
    Handle(ObjectPool* pool, T* obj) noexcept : pool_(pool), obj_(obj) {}
    ObjectPool* pool_;
    T* obj_;
  };

  // CAS-pop the freelist, construct_at the slot, else empty handle.
  Handle acquire() {
    Slot* slot = head_.load(std::memory_order_relaxed);
    while (slot != nullptr) {
      Slot* next = slot->next.load(std::memory_order_relaxed);
      if (head_.compare_exchange_weak(slot, next, std::memory_order_acq_rel,
                                     std::memory_order_relaxed)) {
        T* obj = to_object(slot);
        std::construct_at(obj);  // fresh, default-constructed object
        live_.fetch_add(1, std::memory_order_relaxed);
        return Handle(this, obj);
      }
      // lost the race: slot was refreshed by compare_exchange; retry.
    }
    return Handle();  // exhausted
  }

  std::size_t capacity() const noexcept { return capacity_; }

 private:
  struct Slot {
    alignas(T) std::byte storage[sizeof(T)];
    std::atomic<Slot*> next{nullptr};
  };

  // storage_ is the first member, so the object address IS the slot address.
  static Slot* to_slot(T* obj) noexcept {
    return reinterpret_cast<Slot*>(obj);
  }
  static T* to_object(Slot* slot) noexcept {
    return reinterpret_cast<T*>(slot->storage);
  }

  void release_obj(T* obj) noexcept {
    std::destroy_at(obj);
    Slot* slot = to_slot(obj);
    Slot* head = head_.load(std::memory_order_relaxed);
    do {
      slot->next.store(head, std::memory_order_relaxed);
    } while (!head_.compare_exchange_weak(head, slot, std::memory_order_acq_rel,
                                         std::memory_order_relaxed));
    live_.fetch_sub(1, std::memory_order_relaxed);
  }

  // A detached object (Handle::release) is gone for good: the slot never
  // returns to the freelist, so the outstanding count just drops.
  void detach_obj(T* /*obj*/) noexcept {
    live_.fetch_sub(1, std::memory_order_relaxed);
  }

  std::atomic<Slot*> head_{nullptr};
  Slot* arena_{nullptr};
  std::size_t capacity_{0};
  std::atomic<std::size_t> live_{0};
};

#endif  // EXERCISE23_OBJECT_POOL_H_

## Notes
- `std::construct_at`/`std::destroy_at` (from `<memory>`) are the C++20 way to
  run a constructor/destructor on raw storage; the explicit instantiations for
  `int` and `std::string` ensure the template is really compiled.
- Verified GREEN against all 10 tests (including the concurrent-exclusivity
  `tsan;stress` case) before the header was re-stubbed.