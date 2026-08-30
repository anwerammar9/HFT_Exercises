# Exercise 32 — Varint Message Codec (Reference Solution)

**What you implement:** a LEB128 varint codec plus a length-prefixed frame codec
that is strict about buffer bounds and returns 0/`false`/`nullopt` on every
malformed input — never exceptions, never OOB.

**Approach**
- One internal `write_varint(v, out)` (assumes space, for use by both the public
  `encode_varint` and `encode_frame`) and one internal `read_varint(buf, len,
  out)` returning the byte count consumed (0 = malformed/truncated), shared by
  the public `decode_varint` and `decode_frame`.
- Encode: `do { byte = v & 0x7F; v >>= 7; if (v) byte |= 0x80; } while (v);` —
  the `do/while` guarantees 0 encodes as a single `0x00`. Public wrappers check
  `varint_len(v) <= cap` up front (no partial writes on a short buffer).
- Decode: accumulate 7-bit groups (shift += 7) per byte, stop at the first byte
  with MSB clear (the terminator). Malice rules, in order:
  - `i >= 10` → more than 10 bytes → fail;
  - on the **10th** byte (`i == 9`, shift 63) the byte value must be ≤ 1
    (`byte & 0xFE == 0`) or the result would overflow `uint64` → fail;
  - end of buffer before a terminator → truncated → fail.
- Frame: little-endian `uint32` payload length out of `buf[0..3]`, then the uid
  varint, then the payload. `decode_frame` validates: header present, uid
  varint valid, `4 + uid_bytes + plen <= len`, then copies the payload.
- `frame_size` is `4 + varint_len(uid) + payload.size()` — the single source of
  truth for sizing, used by `encode_frame` for both its buffer check and return.

## Reference API — `include/varint_codec.h`

```cpp
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
```

## Reference implementation — `src/varint_codec.cpp`

```cpp
#include "varint_codec.h"

namespace codec {
namespace {

constexpr std::size_t kMaxVarintBytes = 10;

std::size_t write_varint(std::uint64_t v, std::byte* out) {
  std::size_t n = 0;
  do {
    std::byte b = static_cast<std::byte>(v & 0x7F);
    v >>= 7;
    if (v != 0) b |= static_cast<std::byte>(0x80);
    out[n++] = b;
  } while (v != 0);
  return n;
}

// Byte count consumed (>= 1) on success, 0 on malformed/truncated input.
std::size_t read_varint(const std::byte* buf, std::size_t len,
                        std::uint64_t& out) {
  std::uint64_t result = 0;
  for (std::size_t i = 0; i < len; ++i) {
    if (i >= kMaxVarintBytes) return 0;  // more than 10 bytes
    const unsigned char byte = static_cast<unsigned char>(buf[i]);
    if (i == kMaxVarintBytes - 1) {      // 10th byte: <= 1 or it overflows
      if ((byte & 0xFE) != 0) return 0;
      out = result | (static_cast<std::uint64_t>(byte & 0x01) << 63);
      return i + 1;
    }
    result |= static_cast<std::uint64_t>(byte & 0x7F) << (7 * i);
    if ((byte & 0x80) == 0) {            // terminator
      out = result;
      return i + 1;
    }
  }
  return 0;                              // truncated before a terminator
}

}  // namespace

std::size_t encode_varint(std::uint64_t v, std::byte* out, std::size_t cap) {
  const std::size_t need = varint_len(v);
  if (need > cap) return 0;
  return write_varint(v, out);
}

bool decode_varint(const std::byte* buf, std::size_t len, std::uint64_t& out) {
  return read_varint(buf, len, out) != 0;
}

std::size_t varint_len(std::uint64_t v) {
  std::size_t n = 1;
  while (v >>= 7) ++n;
  return n;
}

std::size_t encode_frame(const Message& m, std::byte* out, std::size_t cap) {
  const std::size_t total = frame_size(m);
  if (total > cap) return 0;

  const std::uint32_t plen = static_cast<std::uint32_t>(m.payload.size());
  out[0] = static_cast<std::byte>(plen & 0xFF);
  out[1] = static_cast<std::byte>((plen >> 8) & 0xFF);
  out[2] = static_cast<std::byte>((plen >> 16) & 0xFF);
  out[3] = static_cast<std::byte>((plen >> 24) & 0xFF);

  std::byte* p = out + 4;
  p += write_varint(m.client_uid, p);
  for (std::size_t i = 0; i < m.payload.size(); ++i) p[i] = m.payload[i];
  return total;
}

std::size_t frame_size(const Message& m) {
  return 4 + varint_len(m.client_uid) + m.payload.size();
}

std::optional<Message> decode_frame(const std::byte* buf, std::size_t len) {
  if (len < 4) return std::nullopt;

  const std::uint32_t plen =
      static_cast<std::uint32_t>(buf[0]) |
      (static_cast<std::uint32_t>(buf[1]) << 8) |
      (static_cast<std::uint32_t>(buf[2]) << 16) |
      (static_cast<std::uint32_t>(buf[3]) << 24);

  std::uint64_t uid = 0;
  const std::size_t uid_bytes = read_varint(buf + 4, len - 4, uid);
  if (uid_bytes == 0) return std::nullopt;

  const std::size_t payload_at = 4 + uid_bytes;
  if (len - payload_at < plen) return std::nullopt;

  Message m;
  m.client_uid = uid;
  m.payload.assign(buf + payload_at, buf + payload_at + plen);
  return m;
}

}  // namespace codec
```

**How the tests verify you:** exact known byte patterns, short-buffer
no-partial-write, truncation/overflow rejection, the ≤1 10th-byte rule, and a
hundred randomized round-trips. The stub returns 0/`nullopt` from everything,
so every encode/decode test is red until the real codec exists.