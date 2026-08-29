#include "varint_codec.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

using codec::Message;
using codec::decode_frame;
using codec::decode_varint;
using codec::encode_frame;
using codec::encode_varint;
using codec::frame_size;
using codec::varint_len;

namespace {

int b(const std::byte& x) { return static_cast<int>(x); }

std::vector<std::byte> ToBytes(std::initializer_list<int> ints) {
  std::vector<std::byte> out;
  out.reserve(ints.size());
  for (int i : ints) out.push_back(static_cast<std::byte>(i));
  return out;
}

bool BytesEqual(const std::byte* got, std::size_t n,
                std::initializer_list<int> want) {
  if (n != want.size()) return false;
  std::size_t i = 0;
  for (int w : want) {
    if (b(got[i]) != w) return false;
    ++i;
  }
  return true;
}

}  // namespace

// --- varint encoding -------------------------------------------------------

TEST(VarintTest, EncodeKnownValues) {
  std::array<std::byte, 16> buf;

  EXPECT_EQ(encode_varint(0, buf.data(), buf.size()), 1u);
  EXPECT_TRUE(BytesEqual(buf.data(), 1, {0x00}));

  EXPECT_EQ(encode_varint(1, buf.data(), buf.size()), 1u);
  EXPECT_TRUE(BytesEqual(buf.data(), 1, {0x01}));

  EXPECT_EQ(encode_varint(127, buf.data(), buf.size()), 1u);
  EXPECT_TRUE(BytesEqual(buf.data(), 1, {0x7F}));

  EXPECT_EQ(encode_varint(128, buf.data(), buf.size()), 2u);
  EXPECT_TRUE(BytesEqual(buf.data(), 2, {0x80, 0x01}));

  EXPECT_EQ(encode_varint(300, buf.data(), buf.size()), 2u);
  EXPECT_TRUE(BytesEqual(buf.data(), 2, {0xAC, 0x02}));

  EXPECT_EQ(encode_varint(16383, buf.data(), buf.size()), 2u);
  EXPECT_TRUE(BytesEqual(buf.data(), 2, {0xFF, 0x7F}));

  EXPECT_EQ(encode_varint(16384, buf.data(), buf.size()), 3u);
  EXPECT_TRUE(BytesEqual(buf.data(), 3, {0x80, 0x80, 0x01}));

  EXPECT_EQ(encode_varint(UINT64_MAX, buf.data(), buf.size()), 10u);
  EXPECT_TRUE(BytesEqual(buf.data(), 10,
                         {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x01}));
}

TEST(VarintTest, EncodeShortBufferReturnsZeroWithoutPartialWrites) {
  std::array<std::byte, 16> buf;
  std::fill(buf.begin(), buf.end(), std::byte{0xEE});  // poison

  EXPECT_EQ(encode_varint(300, buf.data(), /*cap*/ 1), 0u);
  EXPECT_EQ(encode_varint(300, buf.data(), 0), 0u);
  EXPECT_TRUE(BytesEqual(buf.data(), buf.size(),
                         {0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
                          0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE}));
}

TEST(VarintTest, VarintLenValues) {
  EXPECT_EQ(varint_len(0), 1u);
  EXPECT_EQ(varint_len(1), 1u);
  EXPECT_EQ(varint_len(127), 1u);
  EXPECT_EQ(varint_len(128), 2u);
  EXPECT_EQ(varint_len(300), 2u);
  EXPECT_EQ(varint_len(16383), 2u);
  EXPECT_EQ(varint_len(16384), 3u);
  EXPECT_EQ(varint_len((static_cast<uint64_t>(1) << 63) - 1), 9u);
  EXPECT_EQ(varint_len(static_cast<uint64_t>(1) << 63), 10u);
  EXPECT_EQ(varint_len(UINT64_MAX), 10u);
}

TEST(VarintTest, DecodeKnownValues) {
  std::array<std::byte, 16> buf;

  EXPECT_EQ(encode_varint(300, buf.data(), buf.size()), 2u);
  std::uint64_t out = 0;
  EXPECT_TRUE(decode_varint(buf.data(), 2, out));
  EXPECT_EQ(out, 300u);

  out = 0;
  EXPECT_TRUE(decode_varint(ToBytes({0x80, 0x01}).data(), 2, out));
  EXPECT_EQ(out, 128u);

  out = 0;
  EXPECT_TRUE(decode_varint(ToBytes({0xFF, 0x7F}).data(), 2, out));
  EXPECT_EQ(out, 16383u);

  out = 0;
  EXPECT_TRUE(decode_varint(ToBytes({0x00}).data(), 1, out));
  EXPECT_EQ(out, 0u);

  out = 0;
  const std::vector<std::byte> full =
      ToBytes({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01});
  EXPECT_TRUE(decode_varint(full.data(), full.size(), out));
  EXPECT_EQ(out, UINT64_MAX);
}

TEST(VarintTest, DecodeIgnoresTrailingBytes) {
  std::uint64_t out = 0;
  EXPECT_TRUE(decode_varint(ToBytes({0x2A, 0x99, 0x00}).data(), 3, out));
  EXPECT_EQ(out, 42u);
}

TEST(VarintTest, DecodeRejectsTruncatedInput) {
  std::uint64_t out = 123;
  EXPECT_FALSE(decode_varint(ToBytes({}).data(), 0, out));
  EXPECT_FALSE(decode_varint(ToBytes({0x80}).data(), 1, out));  // terminator missing
  EXPECT_FALSE(decode_varint(ToBytes({0x80, 0x80}).data(), 2, out));
  EXPECT_EQ(out, 123u);  // untouched on failure
}

TEST(VarintTest, DecodeRejectsOverflow) {
  std::uint64_t out = 0;
  // 10 bytes, all continuation + 0x7F -> would need > 64 bits.
  EXPECT_FALSE(decode_varint(ToBytes({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                      0xFF, 0xFF, 0xFF})
                                 .data(),
                             10, out));
  // 11 bytes with a valid terminator at the end -> more than 10 bytes.
  EXPECT_FALSE(decode_varint(
      ToBytes({0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01})
          .data(),
      11, out));
}

TEST(VarintTest, RoundTripValues) {
  const std::vector<uint64_t> values = {
      0,
      1,
      127,
      128,
      300,
      16383,
      16384,
      123456789,
      static_cast<uint64_t>(1) << 31,
      (static_cast<uint64_t>(1) << 63) - 1,
      static_cast<uint64_t>(1) << 63,
      UINT64_MAX,
      987654321987654321ULL,
  };
  std::array<std::byte, 16> buf;
  for (auto v : values) {
    ASSERT_EQ(encode_varint(v, buf.data(), buf.size()), varint_len(v));
    std::uint64_t out = 0;
    ASSERT_TRUE(decode_varint(buf.data(), varint_len(v), out)) << "value " << v;
    EXPECT_EQ(out, v);
  }
}

// --- frames -----------------------------------------------------------------

TEST(VarintCodecTest, FrameSizeIsHeaderPlusVarintPlusPayload) {
  Message m{/*uid*/ 42, std::vector<std::byte>(10, std::byte{0})};
  EXPECT_EQ(frame_size(m), 4u + 1u + 10u);

  Message m2{/*uid*/ 300, std::vector<std::byte>(0)};
  EXPECT_EQ(frame_size(m2), 4u + 2u + 0u);
  EXPECT_EQ(varint_len(300), 2u);
}

TEST(VarintCodecTest, EncodeDecodeFrameRoundTrip) {
  Message in{/*uid*/ 300, ToBytes({0xAA, 0xBB, 0xCC, 0x00, 0xFF})};
  std::array<std::byte, 64> buf;

  const std::size_t n = encode_frame(in, buf.data(), buf.size());
  ASSERT_EQ(n, frame_size(in));

  auto out = decode_frame(buf.data(), n);
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->client_uid, 300u);
  EXPECT_EQ(out->payload, in.payload);
}

TEST(VarintCodecTest, KnownFrameBytes) {
  Message m{/*uid*/ 1, std::vector<std::byte>{}};
  std::array<std::byte, 16> buf;
  ASSERT_EQ(encode_frame(m, buf.data(), buf.size()), 5u);
  EXPECT_TRUE(BytesEqual(buf.data(), 5, {0x00, 0x00, 0x00, 0x00, 0x01}));
}

TEST(VarintCodecTest, EncodeFrameShortBufferReturnsZero) {
  Message m{/*uid*/ 300, std::vector<std::byte>(8, std::byte{0})};
  std::array<std::byte, 16> buf;
  ASSERT_GT(frame_size(m), 4u);
  EXPECT_EQ(encode_frame(m, buf.data(), frame_size(m) - 1), 0u);
  EXPECT_EQ(encode_frame(m, buf.data(), 0), 0u);
}

TEST(VarintCodecTest, DecodeFrameRejectsTruncatedOrMalformed) {
  Message in{/*uid*/ 300, ToBytes({0x01, 0x02, 0x03})};
  std::array<std::byte, 64> buf;
  const std::size_t n = encode_frame(in, buf.data(), buf.size());
  ASSERT_GT(n, 4u);

  EXPECT_FALSE(decode_frame(buf.data(), n - 1).has_value());  // short payload
  EXPECT_FALSE(decode_frame(buf.data(), 3).has_value());       // short header
  EXPECT_FALSE(decode_frame(buf.data(), 0).has_value());
  EXPECT_FALSE(decode_frame(ToBytes({0x00, 0x00, 0x00, 0x00}).data(), 4).has_value());
}

TEST(VarintCodecTest, DecodeFrameRejectsBogusHeaderLength) {
  // Claims a 200-byte payload but nothing follows the header/uid.
  const std::vector<std::byte> data = ToBytes({0xC8, 0x00, 0x00, 0x00, 0x01});
  EXPECT_FALSE(decode_frame(data.data(), data.size()).has_value());
}

TEST(VarintCodecTest, RandomFrameRoundTrips) {
  std::uint64_t seed = 0x12345678;
  auto next = [&seed] {
    seed = seed * 6364136223846793005ULL + 1;
    return seed;
  };

  std::array<std::byte, 256> buf;
  for (int t = 0; t < 100; ++t) {
    Message m{next(), {}};
    m.payload.resize(static_cast<std::size_t>(next() % 80));
    for (auto& byte : m.payload) byte = static_cast<std::byte>(next() & 0xFF);

    const std::size_t n = encode_frame(m, buf.data(), buf.size());
    EXPECT_EQ(n, frame_size(m));
    auto out = decode_frame(buf.data(), n);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->client_uid, m.client_uid);
    EXPECT_EQ(out->payload, m.payload);
  }
}