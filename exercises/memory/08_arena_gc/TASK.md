# Exercise memory/08_arena_gc (ex49) — Arena Garbage Collector (Task)

## The problem (in plain words)

The ex07 arena reclaims everything in bulk and nothing in between. Grow it a
mark phase: keep bump-order allocation and instant `reset()`, but add roots +
`tracer` edges + `collect()`. The catch that makes this its own exercise is
the **reclaim rule**: collection pops only the *dead tail* — interior garbage
stays pinned until everything above it dies, because objects never move.
That pinning cost (fragmentation without compaction) is the lesson; the full
sweep-everywhere collector is ex50.

## Requirements (what the tests check)

1. Fresh heap: `live() == 0`, `collect() == 0`, capacity as constructed.
2. A rooted object survives `collect()` (freed count 0, still live).
3. An unrooted tail pops: freed count exact, survivors live, `live()` exact.
4. Interior garbage is NOT reclaimed while the tail above it stays live
   (pinned — `collect()` returns 0, both still live).
5. A reachable chain (`next` edges from one root) fully survives.
6. Dropping all roots then collecting drains the whole tail.
7. Allocation past capacity returns a null handle (bump limit enforced).
8. `reset()` frees everything, keeps capacity, and the heap reallocates.
9. Collect is repeatable: full drain, then a no-op collect returning 0.

## Public API

```cpp
struct ArenaGcTracer { void visit(std::size_t slot); ... };
template <typename T> concept ArenaTraceable;  // requires t.trace(tr)
class ArenaGcSlot { virtual void trace(ArenaGcTracer&) const = 0; bool marked; };
template <typename T> class ArenaPtr {  // non-owning handle, copyable
  T* get() const noexcept; T& operator*() const noexcept; ...;
  std::size_t slot() const noexcept;    // for trace(): tr.visit(child.slot())
};
template <typename T> class ArenaRoot {  // RAII root guard, move-only
  ArenaPtr<T> get() const noexcept;
};
class ArenaGcHeap {
  explicit ArenaGcHeap(std::size_t capacity);
  template <typename T, typename... Args> ArenaPtr<T> allocate(Args&&...);
  template <typename T> ArenaRoot<T> root(ArenaPtr<T>);
  std::size_t collect();      // mark from roots, pop dead tail, unmark
  bool is_live(std::size_t) const noexcept; template version for handles
  std::size_t live() const noexcept; std::size_t capacity() const noexcept;
  std::size_t roots() const noexcept;
  void reset();
};
// Payload protocol: void trace(ArenaGcHeap::Tracer& tr) const; or nothing (leaf).
```

The stub lives in `include/arena_gc.h` (templates — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- Slots: `vector<unique_ptr<ArenaGcSlot>>` in push-back (bump) order; roots:
  `vector<size_t>` of indices. `ArenaGcSlotFor<T>` holds the object and
  calls `trace()` only `if constexpr (ArenaTraceable<T>)`.
- `collect()`: iterative index worklist from roots (skip null/out-of-range/
  already-marked) → pop the dead tail (`while back unmarked: pop`) → unmark
  survivors → prune root indices past the new end. Indices of survivors never
  shift (only the tail pops), so handles stay valid.
- `root()` pushes the index; `ArenaRoot`'s dtor/move removes exactly one
  registration (`unroot`). Rooting a null handle registers nothing observable
  (mark skips the null sentinel; prune drops it).
- Single-threaded by contract.

## Make it harder (optional — not covered by the tests)

- **Compaction:** after marking, slide survivors down over dead interior
  slots and rewrite handles (the moving-GC design this exercise avoids —
  compare pause costs).
- **Generations:** two arenas (nursery/tenured); survivors of N collects get
  promoted; minor collects scan the nursery only.
- **Weak handles:** a non-rooting `ArenaWeak<T>` with `lock()`-style upgrade
  gated on `is_live()` (safe only at points where no collect can interleave).

## Files

- Stub: `include/arena_gc.h` (templates — implement inline)
- Tests: `test/test_arena_gc.cpp`
- Reference: `SOLUTION.md`
