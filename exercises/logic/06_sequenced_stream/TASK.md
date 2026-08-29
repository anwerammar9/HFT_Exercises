# Exercise 06 — Sequence Gap Detector / Reorder Buffer (Task)

## The problem (in plain words)

A market-data or drop-copy feed numbers every message. The network can reorder
or lose messages, so the handler must: **deliver messages strictly in sequence
order**, **buffer** out-of-order arrivals until their predecessors arrive, and
**report exactly which sequence numbers are missing** so the caller can request
a replay or snapshot. A message is delivered once and only once.

## Requirements (what the tests check)

1. The stream **starts expecting seq 1**.
2. `on_message(seq, msg)` returns the messages that can now be delivered, in
   order, in a single vector:
   - `seq` is exactly the next expected → deliver it **plus any consecutive
     buffered followers** that began waiting after it (all in one return);
   - `seq` is ahead of the next expected → buffer it and return `{}` (we are
     still waiting on the gap);
   - `seq` is a duplicate (behind the next expected) → return `{}`, never
     deliver it twice.
3. `pending_gaps()` = the missing sequence numbers, ascending, over
   `[next expected, first buffered seq)`. It is non-empty **only** while
   out-of-order messages are sitting in the buffer.

### Worked example
`on_message(3, a)` → buffer; `pending_gaps()` = {1, 2}.
`on_message(1, b)` → deliver {1}; buffer still holds 3;
`pending_gaps()` = {2}.
`on_message(2, c)` → deliver {2, 3} in one call; buffer empty; no gaps.

## Public API

```cpp
using SeqNum = std::uint64_t;
struct Message { SeqNum seq; std::uint64_t payload; };

class SequencedStream {
  std::vector<Message> on_message(SeqNum seq, Message msg);
  std::vector<SeqNum> pending_gaps() const;
};
```

## How to think about it (suggested design)

- Two members: `next_` (next expected sequence number, starts at 1) and a
  `std::map<SeqNum, Message> buffer_` keyed by sequence number (a map keeps the
  buffered messages sorted, which makes gap reporting trivial).
- `on_message`:
  - `seq < next_` → duplicate, ignore;
  - `seq > next_` → buffer it, return `{}`;
  - `seq == next_` → collect it, advance `next_`, then keep consuming every
    consecutive buffered follower while advancing `next_`, and return the
    collected run.
- `pending_gaps()`: walk from `next_` up to `buffer_.begin()→seq` — that whole
  range is missing.
- Single-threaded by design (one feed handler per stream).

## Make it harder (optional — not covered by the tests)

- **Configurable start:** a constructor that takes the first expected seq, and
  a `reset(seq)` to re-anchor after a replay.
- **Bounded buffer:** cap `buffer_`; when it overflows, drop the *oldest*
  buffered message and surface a "stream broken, re-sync" signal instead of
  filling memory forever.
- **Heartbeat detection:** if no message arrives for N calls (or a wall-clock
  timeout), automatically report the full gap range — the “stale feed” alarm.
- **Two-writer safety:** guard with the pattern from Exercise 02 so one thread
  writes while another reads `pending_gaps()` without torn state.

## Files

- Stub: `src/sequenced_stream.cpp`
- Tests: `test/test_sequenced_stream.cpp`
- Reference: `SOLUTION.md`