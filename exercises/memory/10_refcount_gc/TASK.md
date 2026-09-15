# Exercise memory/10_refcount_gc (ex51) — Reference-Counting GC (Task)

## The problem (in plain words)

CPython runs *both* memory schemes at once: reference counts reclaim acyclic
garbage the instant the last handle drops, and a cycle collector (`gc.collect()`)
reaps the cyclic islands counting cannot see. Rebuild that hybrid: prompt
intrusive counts in every node for the fast path, plus `collect_cycles()`
— internal-vs-external ref analysis feeding a tracing mark — for the rest.
The contrast with ex50 (pure tracing, no counts) is the interview lesson:
promptness costs per-pointer writes; cycles cost a whole collection.

## Requirements (what the tests check)

1. Fresh heap: `live() == 0`, `collect_cycles() == 0`.
2. `make<T>()` owns with `use_count() == 1`; copies share, moves steal.
3. The last handle destroys **immediately** — no collection involved.
4. An acyclic chain dies as a cascade the moment the handles drop (prompt,
   exact, collector returns 0 afterwards).
5. A dropped cycle **lingers** (`alive` unchanged, still `live()`) and is
   freed exactly by `collect_cycles()` (freed count exact).
6. A rooted cycle survives collection; `lock()` upgrades the root.
7. An externally-held cycle survives collection; dropping the last handle
   leaves cyclic garbage that the next `collect_cycles()` frees.
8. `reset()` releases immediately with exact counts; self-assign is safe.

## Public API

```cpp
struct RcTracer { void visit(RcNode*); };
template <typename T> concept RcTraceable;  // requires t.trace(tr)
class RcNode { virtual void trace(RcTracer&) const = 0; std::size_t strong; ... };
template <typename T> class RcPtr {  // owning handle: copies bump, moves steal
  T* get() const noexcept; T& operator*() const noexcept; ...;
  void reset() noexcept;
  std::size_t use_count() const noexcept;
  RcNode* node() const noexcept;    // for trace(): tr.visit(child.node())
};
template <typename T> class RcRooted {  // pin w/o counting, move-only RAII
  RcPtr<T> lock() const;            // upgrade to a counted handle
};
class RcHeap {
  template <typename T, typename... Args> RcPtr<T> make(Args&&...);
  template <typename T> RcRooted<T> root(const RcPtr<T>&);
  std::size_t collect_cycles();   // free heap-internal islands, return count
  template <typename T> bool is_live(const RcPtr<T>&) const noexcept;
  std::size_t live() const noexcept; std::size_t roots() const noexcept;
};
// Payload protocol: void trace(RcTracer& tr) const; or nothing (leaf).
```

The stub lives in `include/refcount_gc.h` (templates — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- Nodes: `vector<unique_ptr<RcNode>>` registry + `unordered_set<RcNode*>`
  live set; `RcObj<T>` holds the value and forwards `trace()` conditionally.
- `RcPtr` holds `(heap, node, ptr)` with the heap type-erased to `void*`
  (breaks the handle/heap cycle); an `RcHeapAccess` bridge calls the heap's
  private `destroy_now`/`unroot`. Drop-to-zero destroys + unregisters
  immediately — the cascade that frees chains for free.
- Roots pin without counting (`RcRooted`, move-only, `lock()` re-bumps).
- `collect_cycles()`: count internal refs (edges from heap nodes), seed with
  roots + nodes with `strong > internal` (refs from outside), mark, sweep
  unmarked, prune dead roots. A node referenced only from inside the heap
  (or nothing) is cyclic garbage.
- Single-threaded by contract (plain counters; atomic counts were ex45).

## Make it harder (optional — not covered by the tests)

- **Weak refs:** `RcWeak<T>` that observes without bumping and expires when
  the node dies (compare ex45's `WeakPtr::lock` CAS upgrade).
- **Trial-deletion colors:** implement the full Bacon–Rajan three-color
  pass instead of the seed rule, and prove equivalence on the test graphs.
- **Promptness audit:** count destructor *timing* (immediate vs collected)
  across shapes — chain, tree, DAG, cycle — and tabulate which shapes ever
  need the collector.

## Files

- Stub: `include/refcount_gc.h` (templates — implement inline)
- Tests: `test/test_refcount_gc.cpp`
- Reference: `SOLUTION.md`
