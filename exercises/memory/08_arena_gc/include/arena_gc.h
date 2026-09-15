#ifndef EXERCISE49_ARENA_GC_H_
#define EXERCISE49_ARENA_GC_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// TODO(anwer): implement the arena collector (see SOLUTION.md).
//
// Contract:
//   - allocate<T>() appends a bump slot (null handle when full).
//   - ArenaRoot<T> is the move-only RAII root guard.
//   - collect() marks from roots, pops the DEAD TAIL only, returns count.
//   - Interior garbage stays pinned; reset() bulk-reclaims.
//
// Suggested shape (see SOLUTION.md):
//   - vector<unique_ptr<ArenaGcSlot>> slots_ (bump order) + vector<size_t> roots_.
//   - Iterative index worklist; pop-while-unmarked; unmark survivors; prune
//     dead root indices; unroot() removes one registration.
template <typename T>
class ArenaPtr;
template <typename T>
class ArenaRoot;

class ArenaGcHeap;

struct ArenaGcTracer {
  explicit ArenaGcTracer(std::vector<std::size_t>* work) : work_(work) {}
  void visit(std::size_t slot) {
    if (slot != kNull) work_->push_back(slot);
  }
  static constexpr std::size_t kNull = static_cast<std::size_t>(-1);

 private:
  std::vector<std::size_t>* work_;
};

template <typename T>
concept ArenaTraceable = requires(const T& t, ArenaGcTracer& tr) {
  t.trace(tr);
};

class ArenaGcSlot {
 public:
  virtual ~ArenaGcSlot() = default;
  virtual void trace(ArenaGcTracer& tr) const = 0;
  bool marked = false;
};

template <typename T>
class ArenaGcSlotFor : public ArenaGcSlot {
 public:
  template <typename... Args>
  explicit ArenaGcSlotFor(Args&&... args) : obj_(std::forward<Args>(args)...) {}
  void trace(ArenaGcTracer& tr) const override {
    if constexpr (ArenaTraceable<T>) obj_.trace(tr);
  }
  T obj_;
};

template <typename T>
class ArenaPtr {
 public:
  ArenaPtr() noexcept : heap_(nullptr), slot_(ArenaGcTracer::kNull), ptr_(nullptr) {}
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(const ArenaPtr& o) const noexcept { return ptr_ == o.ptr_; }

  std::size_t slot() const noexcept { return slot_; }

 private:
  friend class ArenaGcHeap;
  template <typename U>
  friend class ArenaRoot;
  ArenaPtr(ArenaGcHeap* heap, std::size_t slot, T* p) noexcept
      : heap_(heap), slot_(slot), ptr_(p) {}

  ArenaGcHeap* heap_;
  std::size_t slot_;
  T* ptr_;
};

template <typename T>
class ArenaRoot {
 public:
  ArenaRoot() noexcept = default;
  ArenaRoot(const ArenaRoot&) = delete;
  ArenaRoot& operator=(const ArenaRoot&) = delete;
  ArenaRoot(ArenaRoot&& o) noexcept  // TODO(anwer): transfer registration
      : heap_(nullptr), slot_(ArenaGcTracer::kNull), ptr_(nullptr) {
    (void)o;
  }
  ArenaRoot& operator=(ArenaRoot&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~ArenaRoot() {}  // TODO(anwer): unroot

  ArenaPtr<T> get() const noexcept { return ArenaPtr<T>(heap_, slot_, ptr_); }

 private:
  friend class ArenaGcHeap;
  ArenaRoot(ArenaGcHeap* heap, std::size_t slot, T* p) noexcept
      : heap_(heap), slot_(slot), ptr_(p) {}

  ArenaGcHeap* heap_ = nullptr;
  std::size_t slot_ = ArenaGcTracer::kNull;
  T* ptr_ = nullptr;
};

class ArenaGcHeap {
 public:
  using Tracer = ArenaGcTracer;

  explicit ArenaGcHeap(std::size_t capacity) : capacity_(capacity) {}

  ArenaGcHeap(const ArenaGcHeap&) = delete;
  ArenaGcHeap& operator=(const ArenaGcHeap&) = delete;

  template <typename T, typename... Args>
  ArenaPtr<T> allocate(Args&&... /*args*/) {  // TODO(anwer): bump-append
    return ArenaPtr<T>();
  }

  template <typename T>
  ArenaRoot<T> root(ArenaPtr<T> /*h*/) {  // TODO(anwer): register + guard
    return ArenaRoot<T>();
  }

  std::size_t collect() {  // TODO(anwer): mark, pop dead tail
    return 0;
  }

  bool is_live(std::size_t /*slot*/) const noexcept {  // TODO(anwer)
    return false;
  }
  template <typename T>
  bool is_live(ArenaPtr<T> /*h*/) const noexcept {  // TODO(anwer)
    return false;
  }

  std::size_t live() const noexcept { return 0; }  // TODO(anwer)
  std::size_t capacity() const noexcept { return capacity_; }
  std::size_t roots() const noexcept { return 0; }  // TODO(anwer)

  void reset() {}  // TODO(anwer): bulk reclaim

 private:
  std::size_t capacity_;
};

#endif  // EXERCISE49_ARENA_GC_H_
