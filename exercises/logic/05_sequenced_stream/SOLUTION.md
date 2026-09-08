# Exercise logic/05_sequenced_stream (ex06) — Sequence Gap Detector / Reorder Buffer (Reference Solution)

**What you implement:** a market-data / drop-copy feed handler that buffers
out-of-order messages, delivers strictly in sequence order, and reports which
seq numbers are missing so a caller can request a replay/snapshot.

**Approach**
- State: `next_` (the next expected seq, starts at **1**) and a
  `std::map<SeqNum, Message> buffer_` keyed by seq.
- `on_message(seq, msg)`:
  - `seq < next_` → duplicate/late, ignored (`{}`).
  - `seq > next_` → ahead of sequence, parked in the buffer (`{}`).
  - `seq == next_` → in order: consume; then repeatedly find `next_` in the
    buffer and swallow consecutive followers until a gap, returning the whole
    in-order run in one call (out-of-order test: 3 buffers until 2 arrives,
    then `{2,3}` flush together).
- `pending_gaps()`: empty when nothing is buffered, else the contiguous range
  `[next_, buffer_.begin()->first)` — exactly the missing seqs
  (gap test: buffer {4,5}, next 3 → `{3}`).
- Single-threaded by design (one feed handler owns one stream).

## Reference API — `include/sequenced_stream.h`
#ifndef EXERCISE06_SEQUENCED_STREAM_H_
#define EXERCISE06_SEQUENCED_STREAM_H_

#include <cstdint>
#include <map>
#include <vector>

// Sequence gap detector / reorder buffer for a market-data or drop-copy feed.
//
// Contract:
//   - The stream starts expecting seq 1.
//   - on_message(seq, msg) delivers the message ONLY once it is in order:
//     in-order arrivals (and any now-consecutive buffered followers) are
//     returned; everything ahead of the next expected seq is buffered and the
//     call returns {}.
//   - A duplicate seq is ignored: returns {} and is never delivered twice.
//   - pending_gaps(): ascending seq numbers we are still waiting on, i.e.
//     [next expected, first buffered); non-empty only while out-of-order
//     messages are buffered.
//
// Implementation notes (reference solution):
//   - `next_` = next expected seq; `buffer_` = map<SeqNum, Message> keyed by
//     seq. on_message: seq < next_ -> duplicate, ignore; seq == next_ ->
//     consume it and swallow every consecutive buffered follower while
//     advancing next_; otherwise buffer it. Single-threaded by design (a feed
//     handler owns one stream).
//
// TODO(anwer): implement the stream (see SOLUTION.md). Stub: nothing is ever
// delivered and no gaps are ever reported -> tests run RED.
using SeqNum = std::uint64_t;

struct Message {
  SeqNum seq;
  std::uint64_t payload;
};

class SequencedStream {
 public:
  std::vector<Message> on_message(SeqNum seq, Message msg);
  std::vector<SeqNum> pending_gaps() const;

 private:
  SeqNum next_{1};
  std::map<SeqNum, Message> buffer_;
};

#endif  // EXERCISE06_SEQUENCED_STREAM_H_

## Reference implementation — `src/sequenced_stream.cpp`
#include "sequenced_stream.h"

#include <utility>

// Feed reorder buffer: deliver strictly in sequence order. Messages ahead of
// the next expected seq are parked in a seq-keyed map; arrivals that bridge
// the gap flush the now-consecutive run in one call.

std::vector<Message> SequencedStream::on_message(SeqNum seq, Message msg) {
  if (seq < next_) return {};  // duplicate / late: ignored

  if (seq > next_) {  // ahead of sequence: buffer it
    buffer_.emplace(seq, std::move(msg));
    return {};
  }

  // seq == next_: consume, then swallow every consecutive buffered follower.
  next_ = seq + 1;
  std::vector<Message> out;
  out.reserve(1 + buffer_.size());
  out.push_back(std::move(msg));

  auto it = buffer_.find(next_);
  while (it != buffer_.end()) {
    out.push_back(std::move(it->second));
    buffer_.erase(it);
    ++next_;
    it = buffer_.find(next_);
  }
  return out;
}

std::vector<SeqNum> SequencedStream::pending_gaps() const {
  std::vector<SeqNum> gaps;
  if (!buffer_.empty()) {
    // Everything in [next_, first_buffered) is missing.
    for (SeqNum s = next_; s < buffer_.begin()->first; ++s) gaps.push_back(s);
  }
  return gaps;
}