#ifndef EXERCISE32_VARINT_CODEC_H_
#define EXERCISE32_VARINT_CODEC_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// Variable-length integer + framed-message codec.
//
// varint (LEB128, as in Protobuf): unsigned integers encoded little-endian,
// 7 bits per byte; the high bit (0x80) of each byte means "more bytes
// follow". A uint64 needs at most 10 bytes, and the 10th byte may hold
// at most the value 1.
//
//   value  -> bytes
//   0      -> 0x00
//   1      -> 0x01
//   300    -> 0xAC 0x02
//   1<<63  -> 9 x 0x80 0x01
//
// frame (the wire message on top of the varint):
//   [ 4-byte little-endian payload length ][ varint client_uid ][ payload ... ]
//
//   frame_size() == 4 + varint_len(uid) + payload.size()
//
// Contract (return-value style — explicit, no exceptions on malformed data):
//   - encode_varint / encode_frame: number of bytes written; 0 when the output
//     buffer is too small (nothing is partially written).
//   - decode_varint: true + fills `out` on success; false on truncated or
//     malformed input (an un-terminated byte stream, or an overflow past
//     uint64 - i.e. > 10 bytes, or a 10th byte > 1).
//   - decode_frame: std::nullopt on any failure (too short for the header,
//     malformed uid, payload length out of range, or trailing bytes missing).
//
// TODO(anwer): implement in src/varint_codec.cpp.
//   - write loop: do { byte = v & 0x7F; v >>= 7; if (v) byte |= 0x80; } while (v);
//   - read loop: shift up 7 bits a byte; fail at >10 bytes; the 10th byte
//     (shift == 63) must be <= 1; stop when the MSB is clear; truncated
//     (ran out of buffer before a terminator) -> false.
//   - frame: 4 header bytes little-endian, then varint uid, then payload.

namespace codec {

struct Message {
  std::uint64_t client_uid;
  std::vector<std::byte> payload;
};

// Encodes `v` as a varint into out[0..cap); returns bytes written (1..10),
// or 0 if cap is too small.
std::size_t encode_varint(std::uint64_t v, std::byte* out, std::size_t cap);

// Decodes one varint from buf[0..len); true + `out` on success, false on
// truncated/malformed input. Ignores any bytes after the varint's terminator.
bool decode_varint(const std::byte* buf, std::size_t len, std::uint64_t& out);

// Number of bytes encode_varint would write (1..10).
std::size_t varint_len(std::uint64_t v);

// Encodes a Message into its frame in out[0..cap); returns the frame size in
// bytes, or 0 if cap is too small.
std::size_t encode_frame(const Message& m, std::byte* out, std::size_t cap);

// Total frame size in bytes (frame_size() == encode_frame's return).
std::size_t frame_size(const Message& m);

// Decodes a framed Message from buf[0..len); nullopt on any malformation.
std::optional<Message> decode_frame(const std::byte* buf, std::size_t len);

}  // namespace codec

#endif  // EXERCISE32_VARINT_CODEC_H_