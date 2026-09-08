# Exercise domain/02_order_gateway (ex16) — Idempotent Order Gateway (Task)

## The problem (in plain words)

A client can retransmit an order (network retry, double-click) and the broker
must not receive it twice. An **idempotent gateway** keys submissions by
`ClientOrderId`: the first `submit(cid, order)` forwards the order; any
resubmission of the **same cid within a dedup window** is silently dropped.
After the window elapses the cid may be used for a *new* order. All of this
must hold with many threads submitting at once, and the "clock" must be
injectable so tests never sleep.

## Requirements (what the tests check)

1. `submit(cid, order)` returns `true` and forwards `order` to the sink **only
   the first time** that `cid` is seen inside `dedup_window`.
2. A resubmission inside the window returns `false` and is **NOT forwarded**
   again.
3. Once `dedup_window` elapses (per the injectable clock), the same `cid` may be
   reused for a NEW order and is forwarded again.
4. **Thread safety:** concurrent `submit` calls for one cid must resolve to
   **exactly one** success — this is the classic *check-then-act* race; the
   "seen" state must be gated atomically (CAS/lock), not read-then-written.
5. **Clock:** `submit()` asks a `now()` function for the current time in
   nanoseconds (steady by default). Tests inject a mock
   `std::atomic<std::uint64_t>` clock so the window can be advanced without
   sleeping.

## Public API

```cpp
using ClientOrderId = std::uint64_t;
using Price = std::int64_t;
using OrderQty = std::int64_t;
enum class OrderSide { Buy, Sell };
struct Order { OrderSide side; Price price; OrderQty qty; };

class OrderGateway {
  using Sink = std::function<void(const Order&)>;
  using NowFn = std::function<std::uint64_t()>;

  OrderGateway(std::chrono::nanoseconds dedup_window, Sink sink,
               NowFn now = NowFn(&OrderGateway::DefaultClock));
  bool submit(ClientOrderId cid, const Order& order);
};
```

## How to think about it (suggested design)

- A `bucket = now()/dedup_window` set keeps the **current** and **previous**
  bucket only — any id older than that is expired and can be forgotten. This
  gives amortized O(1) TTL eviction with **no per-entry timers** (a dirt-simple
  alternative to Exercise 09's LRU, and the right call here: entries age out
  together, there's no recency order to exploit).
- Keep `{bucket → set of live ids}`; `submit` performs *one* guarded operation:
  under the single mutex (or via CAS) do lookup → decided-duplicate? → upsert →
  forward to sink. Critical: the upsert and the forward must happen while the
  same mutex is held, so two threads resubmitting the same cid serialize to one
  winner.
- The mock clock is just a plain `std::atomic<std::uint64_t>` the tests write.

## Make it harder (optional — not covered by the tests)

- **Notify-sink dedup:** pass each id to the sink *once*, but return per-call
  outcomes — prove no sink invocation is dropped even when the winning thread
  dies before forwarding.
- **Explicit window compaction:** a `compact()` you call periodically instead of
  the lazy current/previous buckets, and a metric for how many ids were swept.
- **Per-cid rejection cache:** also remember *rejected* ids so a resubmission of
  a rejected order is not re-sent to the exchange — never *escalate* a
  rejection.
- **Reuse-after-window test hardening:** hammer the same cid after the window
  moves and assert forwardship resets exactly at the boundary tick.
- **Persistence mirror:** a forward log (`(cid, ts)` append-only) so a crash can
  be replayed without double-forwarding.

## Files

- Stub: `src/order_gateway.cpp`
- Tests: `test/test_order_gateway.cpp`
- Reference: `SOLUTION.md`