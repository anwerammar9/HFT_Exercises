# Exercise memory/09_mark_sweep (ex50) — Mark-Sweep Collector (Task)

## The problem (in plain words)

Reference counting (ex45, ex51) frees acyclic garbage instantly but chokes on
cycles: two objects pointing at each other keep each other alive forever.
The classic answer is **tracing**: forget counts, start from the roots, mark
everything reachable by following `trace()` edges, and sweep the rest —
cycles die like everything else. Implement the full mark-sweep heap: stable
slots, iterative worklist marking, exact sweeping anywhere in the heap.

## Requirements (what the tests check)

1. Fresh heap: `live() == 0`, `collect() == 0`.
2. A rooted object survives collection untouched.
3. Unrooted objects are swept **anywhere** in the heap — garbage on both
   sides of a survivor is reclaimed (the anti-ex49 test).
4. A reachable chain from one root fully survives (transitive marking).
5. An **unreachable cycle is reclaimed** (freed count exact, both dead) —
   the test refcounting fails.
6. A rooted cycle fully survives.
7. Dropping the root then collecting frees the object.
8. A doubly-rooted shared child survives one collect, then dies with both
   roots dropped.

## Public API

```cpp
struct GcTracer { void visit(GcSlot*); };
template <typename T> concept GcTraceable;  // requires t.trace(tr)
class GcSlot { virtual void trace(GcTracer&) const = 0; bool marked; };
template <typename T> class GcPtr {  // non-owning handle, copyable
  T* get() const noexcept; T& operator*() const noexcept; ...;
  GcSlot* slot_ptr() const noexcept;  // for trace(): tr.visit(child.slot_ptr())
};
template <typename T> class GcRooted {  // RAII root guard, move-only
  GcPtr<T> get() const noexcept;
};
class GcHeap {
  template <typename T, typename... Args> GcPtr<T> allocate(Args&&...);
  template <typename T> GcRooted<T> root(GcPtr<T>);
  std::size_t collect();      // mark from roots, sweep unmarked, unmark
  template <typename T> bool is_live(GcPtr<T>) const noexcept;
  std::size_t live() const noexcept; std::size_t roots() const noexcept;
};
// Payload protocol: void trace(GcTracer& tr) const; or nothing (leaf).
```

The stub lives in `include/mark_sweep.h` (templates — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- Slots: `std::list<unique_ptr<GcSlot>>` (addresses stable across sweeps) +
  an `unordered_set<GcSlot*>` live set so `is_live()` never derefs freed
  memory. Roots: `vector<GcSlot*>`.
- `GcSlotFor<T>` holds the object; `trace()` forwards only
  `if constexpr (GcTraceable<T>)`.
- `collect()`: seed the worklist with roots, mark iteratively (skip null /
  already-swept / already-marked), then erase every unmarked slot from the
  list *and* the live set, unmark survivors. Any entry still naming a slot
  afterwards is live by construction (freed slots had no live root).
- `GcRooted`'s dtor/move removes exactly one root registration.
- Single-threaded by contract. Handles dangle after their object is swept —
  `is_live()` is the guard; never deref across a collect without it.

## Make it harder (optional — not covered by the tests)

- **Tri-color incremental:** split marking into bounded `mark_step(n)` slices
  (grey worklist) so no single collect pauses the world — the latency fix
  HFT would demand, and why real-time systems still avoid GC.
- **Weak handles:** `GcWeak<T>` naming a slot without rooting it, with an
  `is_live()`-gated upgrade (safe only where collect cannot interleave).
- **Finalizers:** per-slot `destroy` hooks that run pre-sweep for resource
  release ordering (compare C++ destructor determinism).

## Files

- Stub: `include/mark_sweep.h` (templates — implement inline)
- Tests: `test/test_mark_sweep.cpp`
- Reference: `SOLUTION.md`
