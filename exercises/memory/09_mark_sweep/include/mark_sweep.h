#ifndef EXERCISE50_MARK_SWEEP_H_
#define EXERCISE50_MARK_SWEEP_H_

#include <cstddef>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

// TODO(anwer): implement the mark-sweep collector (see SOLUTION.md).
//
// Contract:
//   - allocate<T>() creates an object, returns a non-owning handle.
//   - GcRooted<T> is the move-only RAII root guard.
//   - collect() marks from roots (following trace() edges) and sweeps
//     everything unmarked ANYWHERE; cycles die like everything else.
//   - is_live(h) guards handles across collections.
//
// Suggested shape (see SOLUTION.md):
//   - list<unique_ptr<GcSlot>> (stable addresses) + unordered_set live set.
//   - Iterative pointer worklist; skip null/swept/marked; erase unmarked
//     from list AND set; unmark survivors; unroot() removes one registration.
template <typename T>
class GcPtr;
template <typename T>
class GcRooted;

class GcSlot;

struct GcTracer {
  explicit GcTracer(std::vector<GcSlot*>* work) : work_(work) {}
  void visit(GcSlot* s) {
    if (s != nullptr) work_->push_back(s);
  }

 private:
  std::vector<GcSlot*>* work_;
};

template <typename T>
concept GcTraceable = requires(const T& t, GcTracer& tr) { t.trace(tr); };

class GcSlot {
 public:
  virtual ~GcSlot() = default;
  virtual void trace(GcTracer& tr) const = 0;
  bool marked = false;
};

template <typename T>
class GcSlotFor : public GcSlot {
 public:
  template <typename... Args>
  explicit GcSlotFor(Args&&... args) : obj_(std::forward<Args>(args)...) {}
  void trace(GcTracer& tr) const override {
    if constexpr (GcTraceable<T>) obj_.trace(tr);
  }
  T obj_;
};

template <typename T>
class GcPtr {
 public:
  GcPtr() noexcept : slot_(nullptr), ptr_(nullptr) {}
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(const GcPtr& o) const noexcept { return ptr_ == o.ptr_; }

  GcSlot* slot_ptr() const noexcept { return slot_; }

 private:
  friend class GcHeap;
  template <typename U>
  friend class GcRooted;
  GcPtr(GcSlot* slot, T* p) noexcept : slot_(slot), ptr_(p) {}

  GcSlot* slot_;
  T* ptr_;
};

class GcHeap;

template <typename T>
class GcRooted {
 public:
  GcRooted() noexcept = default;
  GcRooted(const GcRooted&) = delete;
  GcRooted& operator=(const GcRooted&) = delete;
  GcRooted(GcRooted&& o) noexcept  // TODO(anwer): transfer registration
      : heap_(nullptr), slot_(nullptr), ptr_(nullptr) {
    (void)o;
  }
  GcRooted& operator=(GcRooted&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~GcRooted() {}  // TODO(anwer): unroot

  GcPtr<T> get() const noexcept { return GcPtr<T>(slot_, ptr_); }

 private:
  friend class GcHeap;
  GcRooted(GcHeap* heap, GcSlot* slot, T* p) noexcept
      : heap_(heap), slot_(slot), ptr_(p) {}

  GcHeap* heap_ = nullptr;
  GcSlot* slot_ = nullptr;
  T* ptr_ = nullptr;
};

class GcHeap {
 public:
  GcHeap() = default;
  GcHeap(const GcHeap&) = delete;
  GcHeap& operator=(const GcHeap&) = delete;

  template <typename T, typename... Args>
  GcPtr<T> allocate(Args&&... /*args*/) {  // TODO(anwer): create + register
    return GcPtr<T>();
  }

  template <typename T>
  GcRooted<T> root(GcPtr<T> /*h*/) {  // TODO(anwer): register + guard
    return GcRooted<T>();
  }

  std::size_t collect() {  // TODO(anwer): mark from roots, sweep unmarked
    return 0;
  }

  template <typename T>
  bool is_live(GcPtr<T> /*h*/) const noexcept {  // TODO(anwer)
    return false;
  }

  std::size_t live() const noexcept { return 0; }  // TODO(anwer)
  std::size_t roots() const noexcept { return 0; }  // TODO(anwer)
};

#endif  // EXERCISE50_MARK_SWEEP_H_
