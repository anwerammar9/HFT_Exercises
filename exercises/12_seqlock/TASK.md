# Exercise 12 — Seqlock + RWSpinLock (Task)

## Problem
Two related reader/writer primitives for the low-latency toolkit: a spin-based
reader/writer lock (Part A) and a sequence-gated snapshot reader that never
blocks and never observes torn data (Part B). Both live in this one exercise.

---

## Part A — `RWSpinLock`

### Requirements (what the tests check)
1. Multiple concurrent readers are allowed (a reader counter proves > 1).
2. A writer has **exclusive** access — a "currently writing" flag proves no
   reader ever overlaps a writer; writer-writer exclusion follows from that.
3. `lock_read()`/`unlock_read()` are thread-paired; same for the write methods.

### Public API
```cpp
class RWSpinLock {
  void lock_read();
  void unlock_read();
  void lock_write();
  void unlock_write();
};
```

### Design notes
Reader count + writer flag. Ordering of flag vs counter matters: a reader must
not slip in on the write path (readers wait for `writer_ == false`; the writer
first waits for `readers_ == 0` and then sets the flag). Spin-based; see the
header's suggested shape.

---

## Part B — `Seqlock<T>`

### Requirements (what the tests check)
1. One writer writes a consistent multi-field payload; a reader polls it
   millions of times and **never** observes a torn state.
2. The reader never blocks and never spins on the writer.
3. Correct memory ordering: the acquire/release sequence protocol must hold
   (the test asserts a payload invariant, e.g. `a + b == sum`, that tears would
   break).

### Public API
```cpp
template <typename T>
class Seqlock {
  T read() const;
  void write(const T& v);
};
```

### Design notes
Snapshot protocol: `read()` loops — load `seq` (acquire); if odd, retry; copy
the payload; reload `seq`; if unchanged, return the copy. `write()` bumps `seq`
to odd (release), stores the payload, bumps to even (release).

## Files
- Stub: `src/rw_spinlock.cpp`, `src/seqlock.cpp`
- Tests: `test/test_seqlock.cpp`
- Reference: `SOLUTION.md`