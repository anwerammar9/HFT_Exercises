# C++ Low-Latency Playground — Citadel Interview Prep

## Goal
A CMake project of 43 self-contained low-latency/concurrency exercises. Each
exercise is its own CMake target built as a GoogleTest binary. History:

- The original spike planned 10 exercises (01–10, C++17); that plan was
  superseded as the project grew.
- Exercises were added over several passes (Part-1 primitives, then a
  from-scratch containers set, then the order/execution domain from the `nnn/`
  spec, then an object pool). Exercise **numbers once reflected creation
  order**, not difficulty.
- The tree was later **renumbered 01–32 in rough difficulty order (easy →
  hard)** so the suite number is a study-order hint, then **grouped by
  theme** (`logic`/`domain`/`memory`/`containers`/`concurrency`). A final
  **design-patterns theme** (`patterns/`, suites **33–37**) was appended —
  factory, strategy, observer, command, chain of responsibility — each with an
  HFT-flavored scenario (venue order creation, pluggable execution, market-data
  fan-out, order-entry undo, and a pre-trade risk pipeline). A final
  **algorithms/data-structures theme** (`algorithms/`, suites **38–43**)
  was appended after the patterns block — an intrusive linked list, the
  two-pointer and sliding-window patterns, a binary search tree, graph search,
  and a from-scratch implicit heap (build-from-range / erase_at / heapsort).
  Directories are numbered **per-category** (`01..NN` inside each theme, e.g.
  `concurrency/01_spinlock`, `algorithms/01_linked_list`) while the global
  suite number (`ex01–ex43`) lives in `TASK.md`/`SOLUTION.md` titles, include
  guards, ctest labels (`exerciseNN`), README/PLAN tables, and cross-exercise
  references.
- Baseline: **C++20** (each target sets `cxx_std_20`).

## How the project works (practice mode)
- Every exercise ships as:
  - `include/<name>.h` — public API, documented with the exercise *contract*.
  - `src/<name>.cpp` — production code, currently **`TODO(anwer)` stubs** that
    compile and fail the tests **RED** (they must never hang or crash).
  - `test/test_<name>.cpp` — the full GoogleTest suite (the contract made
    executable).
  - `SOLUTION.md` — the reference solution: **What you implement** / **Approach**
    plus the header and implementation **verbatim** as verified GREEN during
    the authoring process. (Files under `include/` and `src/` were re-stubbed
    after capture so the repo stays in practice mode.)
- **The flow:** read the contract in the header, implement the algorithm to
  make the tests go GREEN, then diff against `SOLUTION.md`.
- Directories are self-contained (no cross-exercise includes): a header stays
  immutable EXCEPT when a reference implementation legitimately needs new
  private state — the change is captured in `SOLUTION.md`.

## Ground rules for opencode
- Never implement core algorithm logic in exercise `.cpp`/`.h` bodies — keep
  `TODO(anwer)` stubs that compile and fail **red** (return defaults /
  `throw std::logic_error("not implemented")`), never red-because-it-doesn't-
  compile.
- Write GoogleTest suites covering the contract cases; each exercise is its
  own CMake target + `add_test`, so `ctest -R named_test` runs in isolation.
- Keep the concurrency/`stress` suites bounded and deterministic: consumer
  threads use bounded retry loops (`yield()` on contention, attempt caps) —
  tests must fail fast, not hang, under `ctest --timeout 90`.
- Keep the **difficulty-ordered suite numbering (ex01–ex43)** stable: never
  renumber suites unless explicitly asked; when the ordering changes, update
  `TASK.md`/`SOLUTION.md` titles, include guards, ctest labels, README/PLAN
  tables, and cross-exercise references together. Directory names are
  **per-category** (`01..NN` inside each theme) and only encode the position
  inside their theme, ordered by suite number.
- `SOLUTION.md` authoring process (verified-at-write-time): implement the real
  code → build target + run its suite → GREEN → embed the exact bytes → restore
  the stub → final `ctest` re-verifies all suites RED with no hangs/crashes.

## Per-exercise list (ex01 easy → ex43 hard)
| # | Location | Name | Blurb | Labels |
|---|----------|------|-------|--------|
| 01 | logic/01_tick_statistics | tick_statistics | Live mean/var/VWAP/EMA stats for a feed handler | exercise01 |
| 02 | domain/01_order_state_machine | order_state_machine | Strict order lifecycle; CAS-gated transitions | exercise02;tsan;stress |
| 03 | logic/02_twap_vwap_slicer | twap_vwap_slicer | Parent-order slicing with exact integer rounding | exercise03 |
| 04 | logic/03_position_tracker | position_tracker | Average-cost position / realized + unrealized PnL | exercise04 |
| 05 | logic/04_token_bucket | token_bucket | Rate limiter; lazy refill + injectable logical clock | exercise05;tsan;stress |
| 06 | logic/05_sequenced_stream | sequenced_stream | Sequence gap detector / reorder buffer for feeds | exercise06 |
| 07 | memory/01_arena_allocator | arena_allocator | Bump-style arena allocator | exercise07 |
| 08 | memory/02_memory_pool_allocator | memory_pool_allocator | Fixed-size pool allocator (placement new, intrusive freelist) | exercise08 |
| 09 | containers/01_lru_cache | lru_cache | LRU cache (map + intrusive list) | exercise09 |
| 10 | logic/06_timer_wheel | timer_wheel | Single-level/hierarchical timer wheel, tick-driven | exercise10 |
| 11 | concurrency/01_spinlock | spinlock | TAS/TTAS spinlock with backoff; `std::lock_guard`-able | exercise11 |
| 12 | concurrency/02_seqlock | seqlock | Seqlock + reader-writer spinlock; memory-ordering deep dive | exercise12 |
| 13 | concurrency/03_ring_buffer_spsc | ring_buffer_spsc | SPSC lock-free ring buffer (relaxed vs acquire/release) | exercise13;tsan;stress |
| 14 | concurrency/04_thread_pool | thread_pool | Fixed worker pool, `future` results, exception propagation | exercise14 |
| 15 | containers/02_dynamic_vector | dynamic_vector | From-scratch std::vector clone (contiguity, geometric growth) | exercise15 |
| 16 | domain/02_order_gateway | order_gateway | Idempotent order gateway (client-order-id dedup + TTL) | exercise16;tsan;stress |
| 17 | domain/03_risk_gate | risk_gate | Pre-trade position/notional limits (packed-atomic CAS) | exercise17;tsan;stress |
| 18 | domain/04_l2_order_book | l2_order_book | L2 book: snapshot + incremental updates, checksum | exercise18 |
| 19 | domain/05_order_book | order_book | Price-time priority order book (resting/adding/cancelling) | exercise19 |
| 20 | containers/03_priority_queue | priority_queue | Concurrent priority queue (lock or lock-free heap) | exercise20 |
| 21 | concurrency/05_ring_buffer_mpmc | ring_buffer_mpmc | Bounded MPMC queue (per-slot sequence numbers / Vyukov style) | exercise21;tsan;stress |
| 22 | concurrency/06_lockfree_stack | lockfree_stack | Treiber stack + ABA note (tagged/hazard/epoch) | exercise22;tsan;stress |
| 23 | concurrency/07_object_pool | object_pool | Lock-free object pool with RAII handles | exercise23;tsan;stress |
| 24 | containers/04_hash_map | hash_map | Concurrent / open-addressing hash map | exercise24 |
| 25 | concurrency/08_symbol_table | symbol_table | Concurrent symbol table (fine-grained locking / CAS) | exercise25;tsan;stress |
| 26 | domain/06_matching_engine | matching_engine | Full matching engine over the order book | exercise26 |
| 27 | concurrency/09_semaphore | semaphore | Counting semaphore (mutex + condition_variable) | exercise27;tsan;stress |
| 28 | concurrency/10_spin_barrier | spin_barrier | Sense-reversing N-thread spin barrier | exercise28;tsan;stress |
| 29 | concurrency/11_mcs_lock | mcs_lock | MCS queue lock (per-thread spin flag) | exercise29;tsan;stress |
| 30 | domain/07_ohlcv_aggregator | ohlcv_aggregator | Bucketed OHLCV candle aggregation with gap candles | exercise30 |
| 31 | concurrency/12_serial_executor | serial_executor | Serialized task executor (FIFO, drain/shutdown) | exercise31;tsan;stress |
| 32 | containers/05_varint_codec | varint_codec | LEB128 varint + length-prefixed frame codec | exercise32 |
| 33 | patterns/01_factory_orders | factory_orders | Venue order factories (integer tick quantization) | exercise33 |
| 34 | patterns/02_strategy_execution | strategy_execution | Pluggable order execution (TWAP/VWAP/Sniper) | exercise34 |
| 35 | patterns/03_observer_market_data | observer_market_data | Market-data fan-out with symbol filters | exercise35;tsan;stress |
| 36 | patterns/04_command_order_entry | command_order_entry | Order-entry actions as executable/undoable commands | exercise36 |
| 37 | patterns/05_chain_risk | chain_risk | Chain-of-responsibility pre-trade risk pipeline | exercise37 |
| 38 | algorithms/01_linked_list | linked_list | Intrusive doubly-linked list (O(1) erase, reverse, deep copy) | exercise38 |
| 39 | algorithms/02_two_pointer | two_pointer | Two-pointer patterns (pair sums, merge, dedup, max-area, palindrome) | exercise39 |
| 40 | algorithms/03_sliding_window | sliding_window | Sliding-window patterns (monotonic-queue max, min-subarray, distinct-run) | exercise40 |
| 41 | algorithms/04_binary_search_tree | binary_search_tree | BST: insert/erase (0/1/2-child), min/max, nearest, in-order | exercise41 |
| 42 | algorithms/05_graph_search | graph_search | BFS/DFS order, hop distances, components, cycle detection | exercise42 |
| 43 | algorithms/06_heap | heap | Implicit binary heap: build-from-range, erase_at, replace, heapsort | exercise43 |

## CMake conventions
- Top-level `CMakeLists.txt`: `enable_testing()`, FetchContent GoogleTest, then
  `add_subdirectory(exercises)`.
- `exercises/` groups the dirs **by theme** (`logic`, `domain`, `memory`,
  `containers`, `concurrency`, `patterns`, `algorithms`); each category has its
  own `CMakeLists.txt`
  listing its members in ascending **category-local** number order (which is
  suite-number order within the theme), so a flat alphabetized glob of
  every initial `add_subdirectory(exercises/NN_...)` is no longer needed.
- Each exercise target links `gtest_main`; the per-exercise `CMakeLists.txt`
  sets the ctest labels above and a generous `TIMEOUT`.
- Optional sanitizer CMake options exist (`ENABLE_TSAN` / `ENABLE_ASAN`, off
  by default; mutually exclusive). The primary verification config is the
  plain `build/` dir.

## Suggested build order (easiest correctness proof → hardest)
The global suite numbering (ex01–ex43) already encodes the suggested order.
Themes in sequence: pure-logic/domain building blocks (01–06), allocators & containers
(07–10, 15, 20, 24), concurrency primitives (11–13), coordination (14),
market-data & trading systems (16–19), heavier lock-free
structures (21–23, 25), the matching-engine capstone (26),
coordination/codec capstone block (27–32), a design-patterns
block (33–37), and a closing algorithms/data-structures
block (38–43).