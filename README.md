# C++ Low-Latency Playground — Citadel Interview Prep

Self-contained low-latency / concurrency exercises covering the classic HFT
interview space: synchronization primitives, allocators, lock-free queues and
stacks, container re-implementations, thread coordination, in-memory
market-data structures, a matching engine, an object pool, and an
order/execution domain layer (state machine, risk, throttling, sequencing, L2
book building, TWAP/VWAP, PnL, idempotent gateway), the classic design
patterns behind order/execution plumbing (factory, strategy, observer,
command, chain of responsibility), and the classic algorithms and data
structures every coding screen revisits (linked lists, two pointers, sliding
window, binary search trees, graph search, heaps). Each exercise is its own
CMake target + GoogleTest suite, so `ctest -R <name>` runs it in isolation.

> Exercises carry a global suite number **ex01–ex43 in rough difficulty order**
> (easy → hard): pure-logic/domain building blocks first, then allocators and
> containers, then concurrency primitives and coordination, then the heavier
> lock-free structures and concurrent maps, finishing with the matching-engine
> capstone, a design-patterns theme, and a closing algorithms/data-structures
> theme. On disk each theme numbers its own folders **01..NN** (e.g.
> `concurrency/01_spinlock` is suite ex11, `algorithms/01_linked_list` is
> suite ex38); the global number is what `TASK.md`, ctest `LABELS`, and
> include guards use.

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
exercises/                              # grouped by theme; each theme numbers
                                        # its exercises 01..NN (category-local).
                                        # The global 01–43 suite number lives in
                                        # TASK.md / ctest LABELS / include guards.
  logic/                                # pure computation & scheduling logic (easy)
    01_tick_statistics (ex01), 02_twap_vwap_slicer (ex03),
    03_position_tracker (ex04), 04_token_bucket (ex05),
    05_sequenced_stream (ex06), 06_timer_wheel (ex10)
  domain/                               # order / trading domain layer
    01_order_state_machine (ex02), 02_order_gateway (ex16),
    03_risk_gate (ex17), 04_l2_order_book (ex18), 05_order_book (ex19),
    06_matching_engine (ex26), 07_ohlcv_aggregator (ex30)
  memory/                               # allocators
    01_arena_allocator (ex07), 02_memory_pool_allocator (ex08)
  containers/                           # container & codec re-implementations
    01_lru_cache (ex09), 02_dynamic_vector (ex15), 03_priority_queue (ex20),
    04_hash_map (ex24), 05_varint_codec (ex32)
  concurrency/                          # primitives, coordination, lock-free
    01_spinlock (ex11), 02_seqlock (ex12), 03_ring_buffer_spsc (ex13),
    04_thread_pool (ex14), 05_ring_buffer_mpmc (ex21),
    06_lockfree_stack (ex22), 07_object_pool (ex23), 08_symbol_table (ex25),
    09_semaphore (ex27), 10_spin_barrier (ex28), 11_mcs_lock (ex29),
    12_serial_executor (ex31)
  patterns/                             # design patterns (HFT flavor)
    01_factory_orders (ex33), 02_strategy_execution (ex34),
    03_observer_market_data (ex35), 04_command_order_entry (ex36),
    05_chain_risk (ex37)
  algorithms/                           # coding-screen algorithms & DS
    01_linked_list (ex38), 02_two_pointer (ex39), 03_sliding_window (ex40),
    04_binary_search_tree (ex41), 05_graph_search (ex42), 06_heap (ex43)
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

| Target (ctest name) | Suite (global ex01–ex43) | Location | Difficulty tier | Description |
|---|---|---|---|---|
| `tick_statistics_test` | 01 | logic/01_tick_statistics | easy | Live mean/var/VWAP/EMA stats for a feed handler |
| `order_state_machine_test` | 02 | domain/01_order_state_machine | easy | Atomic order-state machine, CAS transitions |
| `twap_vwap_slicer_test` | 03 | logic/02_twap_vwap_slicer | easy | Parent-order slicers (largest-remainder rounding) |
| `position_tracker_test` | 04 | logic/03_position_tracker | easy | Avg-cost position: realized/unrealized PnL |
| `token_bucket_test` | 05 | logic/04_token_bucket | easy | Order throttling: lazy refill, mock clock, no timers |
| `sequenced_stream_test` | 06 | logic/05_sequenced_stream | easy | Feed gap detector / reorder buffer |
| `arena_allocator_test` | 07 | memory/01_arena_allocator | easy | Bump-pointer arena: chunk chain, align, marks, bulk reclaim |
| `pool_allocator_test` | 08 | memory/02_memory_pool_allocator | easy | Fixed-size freelist pool allocator |
| `lru_cache_test` | 09 | containers/01_lru_cache | easy | Hashmap + intrusive-list LRU cache |
| `timer_wheel_test` | 10 | logic/06_timer_wheel | easy | Tick-driven timer wheel |
| `spinlock_test` | 11 | concurrency/01_spinlock | primitives | TAS/TTAS spinlock with backoff (BasicLockable) |
| `seqlock_test` | 12 | concurrency/02_seqlock | primitives | Seqlock (seq number) + RWSpinLock (reader count) |
| `ring_buffer_spsc_test` | 13 | concurrency/03_ring_buffer_spsc | primitives | SPSC lock-free ring buffer, capacity N-1 |
| `thread_pool_test` | 14 | concurrency/04_thread_pool | coordination | Fixed-worker pool, `submit()` -> `std::future` |
| `dynamic_vector_test` | 15 | containers/02_dynamic_vector | containers | std::vector clone: geometric growth, emplace, raw-ptr iterators |
| `order_gateway_test` | 16 | domain/02_order_gateway | domain | Idempotent gateway: client-id dedup, TTL + exactly-one-win |
| `risk_gate_test` | 17 | domain/03_risk_gate | domain | Pre-trade position/notional limits, CAS check-then-commit |
| `l2_order_book_test` | 18 | domain/04_l2_order_book | domain | Snapshot + incremental L2 book, checksum digest |
| `order_book_test` | 19 | domain/05_order_book | domain | Limit order book: add/cancel/modify/market, top-of-book |
| `priority_queue_test` | 20 | containers/03_priority_queue | containers | Binary-heap priority queue (sift-up/sift-down) |
| `ring_buffer_mpmc_test` | 21 | concurrency/05_ring_buffer_mpmc | lock-free | Vyukov bounded MPMC queue (blocking push) |
| `lockfree_stack_test` | 22 | concurrency/06_lockfree_stack | lock-free | Treiber stack (+ ABA handling) |
| `object_pool_test` | 23 | concurrency/07_object_pool | lock-free | Fixed-capacity object pool: RAII handles, lock-free freelist |
| `hash_map_test` | 24 | containers/04_hash_map | containers | Open-addressed HashMap (unordered_map clone, tombstones) |
| `symbol_table_test` | 25 | concurrency/08_symbol_table | lock-free | Thread-safe two-way symbol<->id interning |
| `matching_engine_test` | 26 | domain/06_matching_engine | capstone | Price-time priority order book |
| `semaphore_test` | 27 | concurrency/09_semaphore | coordination | Counting semaphore (mutex + condition_variable) |
| `spin_barrier_test` | 28 | concurrency/10_spin_barrier | coordination | Sense-reversing N-thread spin barrier |
| `mcs_lock_test` | 29 | concurrency/11_mcs_lock | lock-free | MCS queue lock (scalable, no spinning on a shared cacheline) |
| `ohlcv_aggregator_test` | 30 | domain/07_ohlcv_aggregator | domain | Trade → OHLCV candle aggregation (bucketed, gap candles) |
| `serial_executor_test` | 31 | concurrency/12_serial_executor | coordination | Serialized task executor: FIFO, one-at-a-time, drain/shutdown |
| `varint_codec_test` | 32 | containers/05_varint_codec | containers | LEB128 varint + length-prefixed frame codec |
| `factory_orders_test` | 33 | patterns/01_factory_orders | patterns | Venue order factories: integer tick quantization, strict venue rules |
| `strategy_execution_test` | 34 | patterns/02_strategy_execution | patterns | Strategy pattern: pluggable order execution (TWAP/VWAP/Sniper) |
| `observer_market_data_test` | 35 | patterns/03_observer_market_data | patterns | Observer: symbol-filtered market-data fan-out, safe unsubscribe |
| `command_order_entry_test` | 36 | patterns/04_command_order_entry | patterns | Command: order-entry actions as executable/undoable log |
| `chain_risk_test` | 37 | patterns/05_chain_risk | patterns | Chain of responsibility: pre-trade risk gate pipeline |
| `linked_list_test` | 38 | algorithms/01_linked_list | algorithms | Intrusive doubly-linked list: O(1) erase(it), reverse, deep copy |
| `two_pointer_test` | 39 | algorithms/02_two_pointer | algorithms | Two-pointer: sum pairs, merge, dedup, triple-sum, max-area, palindrome |
| `sliding_window_test` | 40 | algorithms/03_sliding_window | algorithms | Sliding window: monotonic-queue max, sums, min-subarray, distinct-run |
| `binary_search_tree_test` | 41 | algorithms/04_binary_search_tree | algorithms | BST: insert/erase (0/1/2-child), min/max, nearest, in-order |
| `graph_search_test` | 42 | algorithms/05_graph_search | algorithms | BFS/DFS order, hop distances, components, cycle detection |
| `heap_test` | 43 | algorithms/06_heap | algorithms | Implicit heap: build-from-range O(n), erase_at, replace, heapsort |

### Sanitizers (off by default)

```bash
cmake -S . -B build-tsan -DENABLE_TSAN=ON
cmake -S . -B build-asan -DENABLE_ASAN=ON
```

The concurrency suites carry the ctest labels `tsan;stress`
(global suite numbers 02, 05, 13, 16, 17, 21, 22, 23, 25, 27, 28, 29, 31) and
suite 35's observer carries `tsan;stress` (35); the rest are
plain `exerciseNN`. Do not
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
- **27 Semaphore** — a budget counter, not a lock: any thread may `release()`,
  `acquire()` blocks (via `condition_variable`, so spurious wakeups are
  handled) until a token exists. `try_acquire()` never consumes on failure;
  `count()` takes the lock so it's TSan-clean. Tests prove
  at-most-`kLimit` holders under contention.
- **28 Spin barrier** — sense-reversing N-thread barrier (the tests treat N=1,
  N=3, N=6; `generation()` counts completed rounds). `wait()` spins with
  `yield()`; a plain no-op stub lets threads "pass through" immediately, which
  fails the arrival-counting tests fast.
- **29 MCS lock** — queue-based spinlock: each waiter spins on its own
  `locked_` flag (contention never hammers one cacheline). Unlock retry-CASes
  the tail if a successor is about to enqueue; the "lost" unlock spins until
  `next_` appears. Tests include single-node reuse across many rounds.
- **30 OHLCV aggregator** — `add_trade` buckets by `ts/bucket_ms`; the timeline
  starts at bucket 0 with **leading empty candles**, skipped buckets are
  finalized as empty candles, stale (older-bucket) trades are ignored, and
  `roll()` finalizes the current bucket while keeping the same bucket open
  (a same-bucket trade after roll opens a fresh candle at the same `open_time`).
- **31 Serial executor** — fixed single worker, FIFO task order,
  `submit()` returns `std::future` and throws `logic_error` after `shutdown()`.
  Tests verify exactly-one-at-a-time (`peak == 1`), exception propagation (the
  worker survives), reentrant submit-from-task, `drain()`, idempotent
  shutdown, and that the destructor runs every submitted task.
- **32 Varint codec** — LEB128 varints (max 10 bytes; last byte ≤ 0x01) plus a
  bare-bones network frame codec: `[4-byte little-endian payload length][varint
  client_uid][payload]`. Decoders reject truncated/overflowing input and never
  partial-write on a too-small buffer.
- **33 Factory** — a `VenueOrderFactory` per venue: `IexOrderFactory` ticks at
  10 units (`price % 10000 == 0`), `CmeOrderFactory` at 2.5 units
  (`% 2500 == 0`); quantization is integer round-half-up `(p + tick/2)/tick*tick`
  so no float noise leaks into prices. Strict venue rules: `kMarket` forces
  `price == 0`, `kStop` can never be `post_only`. `MakeOrderFactory(name)` is
  case-insensitive and throws on unknown venues (the dispatch the strategy
  exercise pairs with).
- **34 Strategy** — `ExecutionEngine` owns a `unique_ptr<ExecutionStrategy>`
  swapped at runtime via `make_strategy(name, seed, child_size)`: TWAP emits
  `base + remainder` on the **last** slice; VWAP uses **largest-remainder**
  rounding of weighted fractions (ties → earliest index) so children sum
  *exactly* to the parent; Sniper emits one full-size child at slot 0. The
  engine (not the strategy) drives the clock and keeps per-slot composition
  state. Replacing the object under the pointer is the pattern.
- **35 Observer** — a topic with `subscribe`/`unsubscribe` returning stable
  slot ids; `publish(feed, msg)` updates *every* subscriber whose symbol filter
  matches (copy-under-lock, callbacks outside the lock). Two feeds, two
  subscribers, symbol-independent symbols + independent feed history = exactly
  four distinct callback sequences; the stateful observer event orders delta
  subscriptions and sees exactly the events for its feed/symbol. A throwing
  subscriber is dropped via its slot id without wedging later subscribers.
- **36 Command** — every `New`/`Modify`/`Cancel` is a `Command` object with
  `execute()` + `undo()` (plus a stable `describe()`); `ExecuteCommandProcessor`
  submits then logs **only successes** in LIFO order, and `undo_last()` reverses
  just the newest applied command. Modify/Cancel snapshot their prior state
  *inside execute()*, so undoing a modify → cancel chain lands back on the
  original fields (the discriminator test).
- **37 Chain of responsibility** — `RiskHandler::handle` is the non-virtual
  chain algorithm in the base class (run check, short-circuit on rejection,
  forward to `next_`, accept at the tail); concretes only implement `do_check`.
  A `RiskChain` facade appends handlers in order. `MaxNotionalHandler` is the
  stateful node: it accrues `price·qty` on passes (never on rejects) and is
  what makes the pipeline order- and state-sensitive.
- **38 Linked list** — intrusive sentinels (a plain `Link` for `head_`/`tail_`,
  payload `Node : Link`); `begin() == end()` on empty with **zero allocation**;
  `erase(it)` splices out in O(1) with no search and returns the successor
  (the resting-order cancel cost model); `front()`/`back()` throw
  `std::out_of_range` when empty (documented deviation from `std::list` UB);
  `reverse()` swaps `prev`/`next` per node then re-attaches the sentinels;
  move/swap `steal()` the payload region instead of cross-linking two
  containers' sentinels.
- **39 Two-pointer** — opposing-pointer pair sweep (`lo`/`hi`, `lo<hi`
  guarantees distinct positions), cross-array pair with `b` walking backward,
  `merge_sorted` tie-ordered `<=` (a before b), in-place writer/reader dedupe,
  triple-sum = sort-a-copy + fixed first + inner pair sweep, `max_area` only
  ever advances the shorter bar, and strict palindrome.
- **40 Sliding window** — fixed-window max via a **monotonic deque of indices**
  (pop back while `≤ value`), rolling `int64` sums with the leaving-left
  subtract, `k==0` for `max_average_window` throws `std::invalid_argument`;
  variable-window grow-right/shrink-left for the shortest sum-≥-target span and
  the longest ≤-k-distinct run.
- **41 Binary search tree** — recursive helpers pass the slot **by reference**
  (`Node*&`) so splicing is trivial; 2-child erase overwrites with the in-order
  successor and splices it out; `nearest` is a single directional walk tracking
  `|key - stored|` (ties → smaller key); `min`/`max` throw on empty;
  `in_order()` is left/self/right.
- **42 Graph search** — sorted `std::set` adjacency for deterministic order;
  BFS marks seen **at enqueue**; DFS pushes neighbors in **reverse** to match
  recursive first-visit order; `bfs_distance` stamps `dist[]` initialised to
  −1 (unreachable stays −1); undirected `has_cycle` = DFS back edge where the
  seen neighbor is **not the parent** (self-loops count); duplicate edges
  collapse and a self-loop counts once in `edge_count_`.
- **43 Heap** — build-from-range heapifies O(n) by sifting **only the internal
  nodes** down; `erase_at` = swap-with-last then sift-up *and* sift-down
  (one is a no-op; bad index throws); `replace` = overwrite root + sift-down in
  one round-trip; `array()` lets tests assert `std::is_heap` directly;
  `heapsort` = build max-heap + pop-max-to-back over the shrinking prefix.

GoogleTest is fetched via `FetchContent` on first configure (requires network).