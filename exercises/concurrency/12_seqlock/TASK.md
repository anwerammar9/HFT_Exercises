# Exercise 12 — Seqlock + RWSpinLock (Task)

## The problem (in plain words)

Two reader/writer primitives every low-latency toolkit needs, and they
complement each other:

- **Part A — `RWSpinLock`:** a spin-based lock where **any number of readers**
  may hold the lock together, but a **writer** needs exclusive access (no
  reader overlaps a writer, ever).
- **Part B — `Seqlock<T>`:** readers **never block at all**. The writer bumps a
  sequence counter to odd while it writes and back to even when done; a reader
  copies the payload and checks the counter didn't move. If it did, the reader
  simply retries. Readers may occasionally spin/retry, but they never wait on
  or stall the writer.

Both live in this one exercise.

---

## Part A — `RWSpinLock`

### Requirements (what the tests check)

1. Multiple concurrent readers are allowed — a reader counter proves more than
   one reader is inside simultaneously.
2. A writer has **exclusive** access — a "currently writing" flag proves no
   reader ever overlaps a writer; writer-writer exclusion follows from a single
   writer flag.
3. `lock_read()`/`unlock_read()` are paired by the same thread; the write pair
   likewise.

### Public API

```cpp
class RWSpinLock {
  void lock_read();
  void unlock_read();
  void lock_write();
  void unlock_write();
};
```

### How to think about it (suggested design)

- Two atomics: `readers_` (count) and `writer_` (flag).
- `lock_read()`: loop while `writer_` is set (or a writer is waiting — the
  header spells out the exact hazard), then increment `readers_`.
- `lock_write()`: first wait until `readers_ == 0`, then set `writer_`;
  `unlock_write()` clears it (release).
- **The ordering of flag vs. counter is the point:** a writer must never overwrite
  a reader's lead and a reader must never slip in while a writer is pending —
  announce the writer *before* readers are barred, per the header's notes.

---

## Part B — `Seqlock<T>`

### Requirements (what the tests check)

1. One writer writes a consistent multi-field payload; a reader polls it
   millions of times and **never** observes a torn state.
2. The reader never blocks and never spins *on the writer* — its retries are
   brief snapshot retries, not waiting for the writer to finish.
3. The acquire/release sequence protocol holds: the test asserts a payload
   invariant (e.g. `a + b == sum`) that a torn copy would break.

### Public API

```cpp
template <typename T>
class Seqlock {
  T read() const;
  void write(const T& v);
};
```

### How to think about it (suggested design)

- Members: the payload `value_` plus one `std::uint64_t seq_` (mutable, atomic).
- `write(v)`: `seq_.store(seq_+1, release)` (odd → snapshot invalid) → copy
  `v` into `value_` → `seq_.store(seq_+1, release)` (even → snapshot valid).
- `read()`: loop —
  1. `old = seq_.load(acquire)`; if `old` is odd, retry (writer mid-flight);
  2. copy `value_` into a local;
  3. `now = seq_.load()`; if `now != old`, retry (payload changed mid-copy);
  4. return the local copy.

---

## Make it harder (optional — not covered by the tests)

- **Writer starvation guard:** cap how long writers wait behind endless readers
  (if `readers_` doesn't drain), or introduce a "writer priority" flag.
- **Non-atomic payload test:** re-run Part B with a plain struct (non-atomic)
  as `T` and widen the writer to *really* tear — prove the seqlock still catches
  it.
- **Combined primitive:** a `SharedMutex`-style lock built from `RWSpinLock`
  inside and used as the writer gate for a `Seqlock` — then argue why the
  seqlock no longer needs the reader count.
- **Micro-benchmark:** readers-only vs. 1-writer contention through a
  `steady_clock` benchmark, and report the read retry rate under load.

> Watch out: Part B stores the payload as `T value_` directly (not atomic), so
> argument 3 matters — the sequence counter is what makes the copy trustworthy.

## Files

- Stub: `src/rw_spinlock.cpp`, `src/seqlock.cpp`
- Tests: `test/test_seqlock.cpp`
- Reference: `SOLUTION.md`