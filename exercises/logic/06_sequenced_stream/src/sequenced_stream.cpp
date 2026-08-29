#include "sequenced_stream.h"


std::vector<Message> SequencedStream::on_message(SeqNum /*seq*/, Message /*msg*/) {
  return {};  // stub: nothing is ever delivered
}

std::vector<SeqNum> SequencedStream::pending_gaps() const {
  return {};  // stub: never reports a gap
}