#include "sequenced_stream.h"

// TODO(anwer): implement the real stream (see SOLUTION.md).
//
// Suggested shape:
//   on_message(seq, msg):
//     if (seq < next_) return {};              // duplicate / late
//     if (seq > next_) { buffer_[seq] = msg; return {}; }
//     next_ = seq + 1;                          // in order: consume
//     std::vector<Message> out{std::move(msg)}; // then swallow consecutive
//     while ((it = buffer_.find(next_)) != buffer_.end()) {
//         out.push_back(std::move(it->second)); buffer_.erase(it); ++next_; }
//     return out;
//   pending_gaps(): {} if buffer_ empty, else [next_, buffer_.begin()->first - 1].

std::vector<Message> SequencedStream::on_message(SeqNum /*seq*/, Message /*msg*/) {
  return {};  // stub: nothing is ever delivered
}

std::vector<SeqNum> SequencedStream::pending_gaps() const {
  return {};  // stub: never reports a gap
}