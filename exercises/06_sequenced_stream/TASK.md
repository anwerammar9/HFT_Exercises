# Exercise 06 — Sequence Gap Detector / Reorder Buffer (Task)

## Problem
A market-data / drop-copy feed handler that buffers out-of-order messages,
delivers only in sequence order, and reports exactly which seq numbers are
missing so the caller can request a replay/snapshot.

## Requirements (what the tests check)
1. The stream **starts expecting seq 1**.
2. `on_message(seq, msg)`:
   - seq is the next expected → deliver it, **plus any consecutive buffered
     followers**, in order, in one return;
   - seq is ahead of the next expected → buffer it and return `{}`;
   - seq is a duplicate (behind the next expected) → return `{}`, never
     deliver twice.
3. `pending_gaps()` = ascending missing seqs in `[next expected, first
   buffered seq)`. Non-empty **only** while out-of-order messages are buffered.

## Public API
```cpp
using SeqNum = std::uint64_t;
struct Message { SeqNum seq; std::uint64_t payload; };

class SequencedStream {
  std::vector<Message> on_message(SeqNum seq, Message msg);
  std::vector<SeqNum> pending_gaps() const;
};
```

## Design notes
Keep `next_` (next expected) and a `std::map<SeqNum, Message>` buffer. On
`seq == next_`: consume + swallow consecutive followers while advancing
`next_`. Single-threaded by design.

## Files
- Stub: `src/sequenced_stream.cpp`
- Tests: `test/test_sequenced_stream.cpp`
- Reference: `SOLUTION.md`