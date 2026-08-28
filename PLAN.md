# C++ Low-Latency Playground — Citadel Interview Prep

## Goal
A CMake project of 26 self-contained low-latency/concurrency exercises. Each
exercise is its own CMake target built as a GoogleTest binary. History:

- The original spike planned 10 exercises (01–10, C++17); that plan was
  superseded as the project grew.
- Exercises were added over several passes (Part-1 primitives, then a
  from-scratch containers set, then the order/execution domain from the `nnn/`
  spec, then an object pool). Exercise **numbers once reflected creation
  order**, not difficulty.
- The tree was later **renumbered 01–26 in rough difficulty order (easy →
  hard)** so the directory number is a study-order hint. Include guards,
  ctest labels (`exerciseNN`), README/PLAN tables, and cross-exercise
  references all follow the current numbering.
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
- Keep the **difficulty-ordered numbering** stable: never renumber exercises
  unless explicitly asked; when the ordering changes, update directory names,
  include guards, ctest labels, SOLUTION.md titles, README/PLAN tables, and
  cross-exercise references together.
- `SOLUTION.md` authoring process (verified-at-write-time): implement the real
  code → build target + run its suite → GREEN → embed the exact bytes → restore
  the stub → final `ctest` re-verifies all suites RED with no hangs/crashes.

## Per-exercise list (01 easy → 26 hard)
| # | Name | Blurb | Labels |
|---|------|-------|--------|
| 01 | tick_statistics | Live mean/var/VWAP/EMA stats for a feed handler | exercise01 |
| 02 | order_state_machine | Strict order lifecycle; CAS-gated transitions | exercise02;tsan;stress |
| 03 | twap_vwap_slicer | Parent-order slicing with exact integer rounding | exercise03 |
| 04 | position_tracker | Average-cost position / realized + unrealized PnL | exercise04 |
| 05 | token_bucket | Rate limiter; lazy refill + injectable logical clock | exercise05;tsan;stress |
| 06 | sequenced_stream | Sequence gap detector / reorder buffer for feeds | exercise06 |
| 07 | arena_allocator | Bump-style arena allocator | exercise07 |
| 08 | memory_pool_allocator | Fixed-size pool allocator (placement new, intrusive freelist) | exercise08 |
| 09 | lru_cache | LRU cache (map + intrusive list) | exercise09 |
| 10 | timer_wheel | Single-level/hierarchical timer wheel, tick-driven | exercise10 |
| 11 | spinlock | TAS/TTAS spinlock with backoff; `std::lock_guard`-able | exercise11 |
| 12 | seqlock | Seqlock + reader-writer spinlock; memory-ordering deep dive | exercise12 |
| 13 | ring_buffer_spsc | SPSC lock-free ring buffer (relaxed vs acquire/release) | exercise13;tsan;stress |
| 14 | thread_pool | Fixed worker pool, `future` results, exception propagation | exercise14 |
| 15 | dynamic_vector | From-scratch std::vector clone (contiguity, geometric growth) | exercise15 |
| 16 | order_gateway | Idempotent order gateway (client-order-id dedup + TTL) | exercise16;tsan;stress |
| 17 | risk_gate | Pre-trade position/notional limits (packed-atomic CAS) | exercise17;tsan;stress |
| 18 | l2_order_book | L2 book: snapshot + incremental updates, checksum | exercise18 |
| 19 | order_book | Price-time priority order book (resting/adding/cancelling) | exercise19 |
| 20 | priority_queue | Concurrent priority queue (lock or lock-free heap) | exercise20 |
| 21 | ring_buffer_mpmc | Bounded MPMC queue (per-slot sequence numbers / Vyukov style) | exercise21;tsan;stress |
| 22 | lockfree_stack | Treiber stack + ABA note (tagged/hazard/epoch) | exercise22;tsan;stress |
| 23 | object_pool | Lock-free object pool with RAII handles | exercise23;tsan;stress |
| 24 | hash_map | Concurrent / open-addressing hash map | exercise24 |
| 25 | symbol_table | Concurrent symbol table (fine-grained locking / CAS) | exercise25;tsan;stress |
| 26 | matching_engine | Full matching engine over the order book | exercise26 |

## CMake conventions
- Top-level `CMakeLists.txt`: `enable_testing()`, FetchContent GoogleTest, then
  `add_subdirectory(exercises/NN_...)` for all 26 (ascending).
- Each exercise target links `gtest_main`; the per-exercise `CMakeLists.txt`
  sets the ctest labels above and a generous `TIMEOUT`.
- Optional sanitizer CMake options exist (`ENABLE_TSAN` / `ENABLE_ASAN`, off
  by default; mutually exclusive). The primary verification config is the
  plain `build/` dir.

## Suggested build order (easiest correctness proof → hardest)
The directory numbering already encodes the suggested order. Themes
in sequence: pure-logic/domain building blocks (01–06), allocators & containers
(07–10, 15, 20, 24), concurrency primitives (11–13), coordination (14),
market-data & trading systems (16–19), heavier lock-free
structures (21–23, 25), and the matching-engine capstone (26).