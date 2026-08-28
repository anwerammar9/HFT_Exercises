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