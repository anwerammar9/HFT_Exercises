#ifndef EXERCISE23_OBJECT_POOL_H_
#define EXERCISE23_OBJECT_POOL_H_

#include <atomic>
#include <cassert>
#include <cstddef>
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

  explicit ObjectPool(std::size_t capacity) : capacity_(capacity) {}

  ~ObjectPool() {
    assert(live_ == 0);
    // TODO(anwer): free the slot arena.
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

  // TODO(anwer): CAS-pop the freelist, construct_at the slot, else empty.
  Handle acquire();

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
    // TODO(anwer): destroy_at(obj); CAS-push to_slot(obj) onto head_.
    (void)obj;
  }

  std::atomic<Slot*> head_{nullptr};
  Slot* arena_{nullptr};
  std::size_t capacity_{0};
  std::size_t live_{0};
};

template <typename T>
typename ObjectPool<T>::Handle ObjectPool<T>::acquire() {
  return Handle();  // TODO(anwer): acquire never succeeds in the stub
}

#endif  // EXERCISE23_OBJECT_POOL_H_