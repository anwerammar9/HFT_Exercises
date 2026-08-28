#include "sequenced_stream.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include <gtest/gtest.h>

namespace {

Message make_msg(SeqNum seq) { return Message{seq, seq * 7}; }

void expect_single(const std::vector<Message>& got, SeqNum seq) {
  ASSERT_EQ(got.size(), 1u);
  EXPECT_EQ(got[0].seq, seq);
  EXPECT_EQ(got[0].payload, seq * 7);
}

TEST(SequencedStreamTest, InOrderDeliveredImmediately) {
  SequencedStream s;
  expect_single(s.on_message(1, make_msg(1)), 1);  // stub: empty -> red
  expect_single(s.on_message(2, make_msg(2)), 2);
  expect_single(s.on_message(3, make_msg(3)), 3);
  EXPECT_TRUE(s.pending_gaps().empty());
}

TEST(SequencedStreamTest, OutOfOrderBufferedThenFlushed) {
  SequencedStream s;
  expect_single(s.on_message(1, make_msg(1)), 1);

  EXPECT_TRUE(s.on_message(3, make_msg(3)).empty());  // arrives ahead of 2
  EXPECT_EQ(s.pending_gaps(), std::vector<SeqNum>{2});

  auto r = s.on_message(2, make_msg(2));  // 2 and 3 flush together, in order
  ASSERT_EQ(r.size(), 2u);
  EXPECT_EQ(r[0].seq, 2u);
  EXPECT_EQ(r[1].seq, 3u);
  EXPECT_TRUE(s.pending_gaps().empty());
}

TEST(SequencedStreamTest, GapKeepsFollowersBuffered) {
  SequencedStream s;
  expect_single(s.on_message(1, make_msg(1)), 1);
  expect_single(s.on_message(2, make_msg(2)), 2);

  EXPECT_TRUE(s.on_message(4, make_msg(4)).empty());
  EXPECT_TRUE(s.on_message(5, make_msg(5)).empty());
  EXPECT_EQ(s.pending_gaps(), std::vector<SeqNum>{3});

  auto r = s.on_message(3, make_msg(3));  // 3,4,5 flush in order
  ASSERT_EQ(r.size(), 3u);
  EXPECT_EQ(r[0].seq, 3u);
  EXPECT_EQ(r[1].seq, 4u);
  EXPECT_EQ(r[2].seq, 5u);
  EXPECT_TRUE(s.pending_gaps().empty());
}

TEST(SequencedStreamTest, DuplicateIgnored) {
  SequencedStream s;
  expect_single(s.on_message(1, make_msg(1)), 1);
  expect_single(s.on_message(2, make_msg(2)), 2);

  EXPECT_TRUE(s.on_message(2, make_msg(2)).empty());  // not delivered twice
  expect_single(s.on_message(3, make_msg(3)), 3);
}

TEST(SequencedStreamTest, ShuffledStressAllDeliveredOnceInOrder) {
  constexpr int kN = 500;
  std::mt19937 rng(20260601u);  // fixed seed -> deterministic
  std::vector<SeqNum> seqs(kN);
  std::iota(seqs.begin(), seqs.end(), 1);
  std::shuffle(seqs.begin(), seqs.end(), rng);

  SequencedStream s;
  std::vector<Message> delivered;
  for (SeqNum seq : seqs) {
    auto r = s.on_message(seq, make_msg(seq));
    delivered.insert(delivered.end(), r.begin(), r.end());
  }

  ASSERT_EQ(delivered.size(), kN);  // stub: all empty -> red
  for (SeqNum i = 1; i <= kN; ++i) {
    EXPECT_EQ(delivered[i - 1].seq, i);
    EXPECT_EQ(delivered[i - 1].payload, i * 7);
  }
  EXPECT_TRUE(s.pending_gaps().empty());
}

}  // namespace