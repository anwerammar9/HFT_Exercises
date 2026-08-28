# C++ Low-Latency Playground — Citadel Interview Prep

Self-contained low-latency / concurrency exercises covering the classic HFT
interview space: synchronization primitives, allocators, lock-free queues and
stacks, container re-implementations, thread coordination, in-memory
market-data structures, a matching engine, an object pool, and an
order/execution domain layer (state machine, risk, throttling, sequencing, L2
book building, TWAP/VWAP, PnL, idempotent gateway). Each exercise is its own
CMake target + GoogleTest suite, so `ctest -R <name>` runs it in isolation.

> Exercises are numbered **01–26 in rough difficulty order** (easy → hard):
> pure-logic/domain building blocks first, then allocators and containers,
> then concurrency primitives and coordination, then the heavier lock-free
> structures and concurrent maps, finishing with the matching-engine or an
> Object capstone.

> **Status — practice mode:** the implementations have been **removed** from the
> C++ files and replaced with `TODO(anwer)` stubs. The full reference solution
> (approach + exact code) lives in each exercise directory's **`SOLUTION.md`**.
> All suites still **compile**, but every suite now runs **RED** by design — the
> stubs are written to fail deterministically without hanging or crashing, so
> the way to turn a suite green is to re-implement the exercise yourself.
>
> The previous green state (plain / AddressSanitizer / ThreadSanitizer) is
> preserved verbatim in the `SOLUTION.md` files, which embed the *tested* code
> byte-for-byte.

> **Language:** C++20 (project baseline; each target sets `cxx_std_20`).

## Layout

```
exercises/
  easy →
  01_tick_statistics, 02_order_state_machine, 03_twap_vwap_slicer,
  04_position_tracker, 05_token_bucket, 06_sequenced_stream,
  07_arena_allocator, 08_memory_pool_allocator, 09_lru_cache,
  10_timer_wheel,
  ↗ primitives & coordination
  11_spinlock, 12_seqlock, 13_ring_buffer_spsc, 14_thread_pool,
  15_dynamic_vector, 16_order_gateway, 17_risk_gate, 18_l2_order_book,
  19_order_book,
  ↗ heavier lock-free + capstones
  20_priority_queue, 21_ring_buffer_mpmc, 22_lockfree_stack,
  23_object_pool, 24_hash_map, 25_symbol_table, 26_matching_engine
```

Each has `TASK.md` (the problem statement — what to implement and what the
tests check), `include/<name>.h` (public API + contract), `src/<name>.cpp`
(implementation — **stubbed**, see below), `test/test_<name>.cpp` (full
GoogleTest suite), a **`SOLUTION.md`** (the reference solution, kept alongside
so you can compare after attempting), and its own `CMakeLists.txt` (library
target + `<name>_test` target + `add_test`).

### How to practice

1. Pick an exercise. Start from its `TASK.md`, then read the header + test file
   (the contract), and attempt the `TODO(anwer)` spot yourself.
2. Build & run just that suite: `ctest --test-dir build -R ring_buffer_spsc`.
   Expect RED — that's the point. A stub is only "wrong" if it **hangs** or
   **crashes** (open a bug if one does); failures should be assertion/timeout
   free.
3. Compare your attempt with `SOLUTION.md` — it embeds the exact code that was
   green under plain/ASan/TSan builds.
4. Then re-stub the exercise (or read the next one).

> The stubs are deliberately *inert*: they compile, return defaults (no-crash
> nullopts), and throw `std::logic_error("not implemented")` on single-threaded
> mutators. Concurrency exercises use bounded loops / deadline spins so the
> suites fail red quickly (a couple may take ~10s as consumers wait out their
> deadline). Worker threads never throw (that would `std::terminate`).

## Building & running

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure          # all suites in tree
ctest --test-dir build -R ring_buffer_spsc          # one suite
ctest --test-dir build -N                           # list suites
```

| Target (ctest name) | Suite | Difficulty tier | Description |
|---|---|---|---|
| `tick_statistics_test` | 01 | easy | Live mean/var/VWAP/EMA stats for a feed handler |
| `order_state_machine_test` | 02 | easy | Atomic order-state machine, CAS transitions |
| `twap_vwap_slicer_test` | 03 | easy | Parent-order slicers (largest-remainder rounding) |
| `position_tracker_test` | 04 | easy | Avg-cost position: realized/unrealized PnL |
| `token_bucket_test` | 05 | easy | Order throttling: lazy refill, mock clock, no timers |
| `sequenced_stream_test` | 06 | easy | Feed gap detector / reorder buffer |
| `arena_allocator_test` | 07 | easy | Bump-pointer arena: chunk chain, align, marks, bulk reclaim |
| `pool_allocator_test` | 08 | easy | Fixed-size freelist pool allocator |
| `lru_cache_test` | 09 | easy | Hashmap + intrusive-list LRU cache |
| `timer_wheel_test` | 10 | easy | Tick-driven timer wheel |
| `spinlock_test` | 11 | primitives | TAS/TTAS spinlock with backoff (BasicLockable) |
| `seqlock_test` | 12 | primitives | Seqlock (seq number) + RWSpinLock (reader count) |
| `ring_buffer_spsc_test` | 13 | primitives | SPSC lock-free ring buffer, capacity N-1 |
| `thread_pool_test` | 14 | coordination | Fixed-worker pool, `submit()` -> `std::future` |
| `dynamic_vector_test` | 15 | containers | std::vector clone: geometric growth, emplace, raw-ptr iterators |
| `order_gateway_test` | 16 | domain | Idempotent gateway: client-id dedup, TTL + exactly-one-win |
| `risk_gate_test` | 17 | domain | Pre-trade position/notional limits, CAS check-then-commit |
| `l2_order_book_test` | 18 | domain | Snapshot + incremental L2 book, checksum digest |
| `order_book_test` | 19 | domain | Limit order book: add/cancel/modify/market, top-of-book |
| `priority_queue_test` | 20 | containers | Binary-heap priority queue (sift-up/sift-down) |
| `ring_buffer_mpmc_test` | 21 | lock-free | Vyukov bounded MPMC queue (blocking push) |
| `lockfree_stack_test` | 22 | lock-free | Treiber stack (+ ABA handling) |
| `object_pool_test` | 23 | lock-free | Fixed-capacity object pool: RAII handles, lock-free freelist |
| `hash_map_test` | 24 | containers | Open-addressed HashMap (unordered_map clone, tombstones) |
| `symbol_table_test` | 25 | lock-free | Thread-safe two-way symbol<->id interning |
| `matching_engine_test` | 26 | capstone | Price-time priority order book |

### Sanitizers (off by default)

```bash
cmake -S . -B build-tsan -DENABLE_TSAN=ON
cmake -S . -B build-asan -DENABLE_ASAN=ON
```

The concurrency suites carry the ctest labels `tsan;stress`
(02, 05, 13, 16, 17, 21, 22, 23, 25); the rest are plain `exerciseNN`. Do not
combine TSan and ASan (asserted at configure time). `ENABLE_WERROR` is also
available. The optional spinlock micro-benchmark builds with
`-DBUILD_SPINLOCK_BENCH=ON`.

## Per-exercise notes (contract choices the tests assume)

- **01 TickStatistics** — O(1) accumulators for count/min/max/last/mean
  (sum/ssq) and VWAP (Σp·q, Σq); price series retained so `ewma(alpha)` is
  answerable for any alpha; population variance (÷n).
- **02 Order state machine** — strict transition table; every edge resolved by
  ONE CAS on `atomic<OrderState>` so racing fill/cancel produce exactly one
  winner; `Filled`/`Cancelled`/`Rejected` are terminal. The race tests assert
  exactly-one-win and that a polling reader never sees a torn state.
- **03 Slicers** — TWAP: `base = total/n` with the **last slice** absorbing the
  remainder (`send_at_i = d*(i+1)/n`); VWAP: fractions are normalized and
  rounded via **largest remainder** so slices sum *exactly* to total, with
  `send_at` = cumulative bucket end. `n==1` / one fraction → whole order at
  0 ms; skipped/empty inputs → empty plan.
- **04 Position tracker** — averaging position; closes realize PnL; a
  zero-crossing fill splits into close-old + open-new (basis resets at the fill
  price). `unrealized = (mark - avg) * position` (sign correct for shorts);
  flat → both 0 (the tested contract).
- **05 Token bucket** — lazy refill from a logical clock (`advance()` is the
  only time source; **no refill thread/timer**); starts full, refill capped at
  `burst`. Mock-clock tests are exact-one-token-per-interval, so float
  rounding must not leak a token.
- **06 Sequenced stream** — seq-1 start, `on_message` returns the in-order run
  (draining consecutive buffered followers), `pending_gaps()` = the
  `[next, first-buffered)` interval; duplicates ignored. Shuffled-500 stress
  uses a fixed RNG seed.
- **07 Arena allocator** — bump-pointer `allocate(size, align)` with chunk
  chaining; `deallocate` is a documented no-op; `reset()` rewinds and reuses
  the same addresses, `release()` returns everything to the OS;
  `allocate_all()`/`rollback_to(mark)` gives stack-discipline scratch. Tests
  assert same-address-after-reset, mark/rollback address reuse, and alignment
  for `alignas(64)` types.
- **08 Pool allocator** — `allocate()` returns `nullptr` when exhausted (no
  throw), runs the ctor; `deallocate` runs the dtor and reuses the exact slot.
  A concurrent test ships DISABLED unless you make it thread-safe.
- **09 LRU** — `get()` touches recency; `put()` on an existing key refreshes;
  evicts the *untouched* key, not the oldest-inserted.
- **10 Timer wheel** — time advances only via `tick(now)` (monotonic ms),
  granularity 1ms, `kWheelSize = 256` slots/rotation, delays may span multiple
  rotations (wrap-around test).
- **11 Spinlock** — mutual-exclusion is the real discriminator (a no-op lock
  fails the counter test). Optional `spinlock_bench` target.
- **12 Seqlock / RWSpinLock** — a read is *always* a consistent snapshot;
  the torn-state test widens the write window on purpose. The test's protected
  fields are atomics so ThreadSanitizer stays quiet while cross-field tearing
  is still detectable.
- **13 SPSC** — power-of-two `N`, usable capacity **N-1**, monotonically
  increasing head/tail indices (`idx & (N-1)`).
- **14 Thread pool** — `shutdown()` drains then joins; submitting after
  shutdown throws `std::logic_error`.
- **15 Vector** — contiguity (raw-pointer iterators = `data() + i`), geometric
  growth (capacity in `[size, 2*size]`), `shrink_to_fit` exact-fit, placement
  new for elements, move-not-copy on reallocation for `noexcept` movables
  (the `Tracked` test asserts `copies == 0`); copy paths are constrained to
  copy-constructible `T`.
- **16 Order gateway** — client-id dedup with a per-id timestamp map and lazy
  TTL compaction at the top of each `submit()`; the lookup + dedup decision +
  upsert + sink forward all happen under one mutex, so concurrent
  resubmissions of one id serialize to exactly one win. Ids are reusable once
  `now()` (injectable `std::atomic` clock, no real sleep) passes the window.
- **17 Risk gate** — the check-then-act race: position (signed) + gross
  notional packed into one 64-bit atomic and committed with a CAS loop (no
  lock). The concurrency tests prove the committed sum can never pass the
  limit (each thread reserves `limit/N`, total == limit exactly).
- **18 L2 book** — `std::map` per side ordered best-first; `qty == 0` removes a
  level and reveals the next-best; updates are a **no-op before the first
  snapshot** (the chosen contract); `checksum()` is a deterministic digest
  (empty book → 0, same op sequence → same digest).
- **19 Order book** — resting orders with FIFO per price; `market_order`
  sweeps levels best-first at the resting price and reports the remainder;
  `modify_order` preserves **time priority**; cancel keeps the remaining FIFO
  order coherent.
- **20 PriorityQueue** — implicit binary heap in a `std::vector`; `Compare`
  like `std::priority_queue` (default `std::less` ⇒ max-heap); `top()` on an
  empty queue **throws** (documented deviation from UB).
- **21 MPMC** — bounded Vyukov queue; `push` **blocks** on full (the plan chose
  `void push`), `try_pop` never blocks. Verified no-loss/no-dup with `seen[][]`.
- **22 Lock-free stack** — Treiber. **ABA handling is a required design
  decision**: nodes are never returned to the allocator while the stack lives
  (popped nodes are parked on a retired list and freed only at destruction), so
  an address can never be reused and `try_pop` may safely read `head->next`
  before the CAS. A production version would substitute hazard pointers or
  epoch reclamation.
- **23 Object pool** — distinct from 08: fixed-capacity `ObjectPool<T>`
  (`acquire()` hands out an owning RAII `Handle` that self-recycles; returns an
  *empty* handle, not `nullptr`, when exhausted). The pool is synchronized —
  CAS-pop/CAS-push through a lock-free freelist — so exhaustion is always
  respected under contention. `acquire()` `construct_at`s a fresh `T` (slot
  value reset between uses), handles recycle one-and-only-once (move semantics
  tested), `release()` detaches without recycling; object lifetime is
  charge-counted (tests assert ctor/dtor balance).
- **24 HashMap** — open addressing, linear probing, power-of-two grow at
  load 0.7 (`kMaxLoad`), **tombstones** for `erase()` (a reuse decrements the
  tombstone count); `clear()` keeps capacity; iterators visit only filled
  slots. A `BadHash` collision-storm test forces probing.
- **25 SymbolTable** — monotonic ids **never reused**; `lookup()` returns a
  non-owning `string_view` that stays valid until `clear()` (low-latency
  memory retention); internally synchronized. The stress test interns
  `kThreads × 1000` distinct symbols and checks uniqueness + both directions.
- **26 Matching engine** — fills at the *resting* price, FIFO per level,
  multi-level sweeps in one `add_order` call.

GoogleTest is fetched via `FetchContent` on first configure (requires network).