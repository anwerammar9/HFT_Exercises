#ifndef EXERCISE51_REFCOUNT_GC_H_
#define EXERCISE51_REFCOUNT_GC_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// TODO(anwer): implement the hybrid refcount + cycle collector (see SOLUTION.md).
//
// Contract:
//   - make<T>() owns with use_count() == 1; copies bump, moves steal.
//   - Drop-to-zero destroys IMMEDIATELY (acyclic garbage needs no collector).
//   - RcRooted<T> pins without counting (move-only); lock() re-bumps.
//   - collect_cycles() frees heap-internal islands (strong == internal,
//     unrooted); rooted/held cycles survive; returns the freed count.
//
// Suggested shape (see SOLUTION.md):
//   - RcNode { strong, marked, virtual trace() }; vector registry + live set.
//   - RcPtr { heap (type-erased), node, ptr } + RcHeapAccess bridge.
//   - collect_cycles(): internal counts → seed (roots + strong > internal) →
//     mark → detach-then-destroy sweep (drops suppressed while sweeping_) →
//     prune dead roots.
template <typename T>
class RcPtr;
template <typename T>
class RcRooted;

class RcNode;

struct RcTracer {
  explicit RcTracer(std::vector<RcNode*>* work) : work_(work) {}
  void visit(RcNode* n) {
    if (n != nullptr) work_->push_back(n);
  }

 private:
  std::vector<RcNode*>* work_;
};

template <typename T>
concept RcTraceable = requires(const T& t, RcTracer& tr) { t.trace(tr); };

class RcNode {
 public:
  virtual ~RcNode() = default;
  virtual void trace(RcTracer& tr) const = 0;
  std::size_t strong = 0;
  bool marked = false;
};

template <typename T>
class RcObj : public RcNode {
 public:
  template <typename... Args>
  explicit RcObj(Args&&... args) : value_(std::forward<Args>(args)...) {}
  void trace(RcTracer& tr) const override {
    if constexpr (RcTraceable<T>) value_.trace(tr);
  }
  T value_;
};

template <typename T>
class RcPtr {
 public:
  RcPtr() noexcept : node_(nullptr), ptr_(nullptr) {}
  RcPtr(const RcPtr& o) noexcept : node_(nullptr), ptr_(nullptr) {  // TODO: bump
    (void)o;
  }
  RcPtr(RcPtr&& o) noexcept : node_(nullptr), ptr_(nullptr) {  // TODO: steal
    (void)o;
  }
  RcPtr& operator=(const RcPtr& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  RcPtr& operator=(RcPtr&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~RcPtr() {}  // TODO(anwer): drop (destroy at zero)

  void reset() noexcept {}  // TODO(anwer)

  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(const RcPtr& o) const noexcept { return ptr_ == o.ptr_; }
  bool operator!=(const RcPtr& o) const noexcept { return ptr_ != o.ptr_; }

  std::size_t use_count() const noexcept { return 0; }  // TODO(anwer)

  RcNode* node() const noexcept { return node_; }

 private:
  RcNode* node_;
  T* ptr_;
};

template <typename T>
class RcRooted {
 public:
  RcRooted() noexcept = default;
  RcRooted(const RcRooted&) = delete;
  RcRooted& operator=(const RcRooted&) = delete;
  RcRooted(RcRooted&& o) noexcept {  // TODO(anwer)
    (void)o;
  }
  RcRooted& operator=(RcRooted&& o) noexcept {  // TODO(anwer)
    (void)o;
    return *this;
  }
  ~RcRooted() {}

  RcPtr<T> lock() const {  // TODO(anwer): re-bump iff pinned
    return RcPtr<T>();
  }
};

class RcHeap {
 public:
  RcHeap() = default;
  RcHeap(const RcHeap&) = delete;
  RcHeap& operator=(const RcHeap&) = delete;

  template <typename T, typename... Args>
  RcPtr<T> make(Args&&... /*args*/) {  // TODO(anwer): create, count 1
    return RcPtr<T>();
  }

  template <typename T>
  RcRooted<T> root(const RcPtr<T>& /*h*/) {  // TODO(anwer): pin w/o counting
    return RcRooted<T>();
  }

  std::size_t collect_cycles() {  // TODO(anwer): free heap-internal islands
    return 0;
  }

  template <typename T>
  bool is_live(const RcPtr<T>& /*h*/) const noexcept {  // TODO(anwer)
    return false;
  }

  std::size_t live() const noexcept { return 0; }  // TODO(anwer)
  std::size_t roots() const noexcept { return 0; }  // TODO(anwer)
};

#endif  // EXERCISE51_REFCOUNT_GC_H_
