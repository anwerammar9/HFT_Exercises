#include "varint_codec.h"

// TODO(anwer): implement the codec (see SOLUTION.md).
//
//   encode_varint(v, out, cap):
//     need = varint_len(v); if (need > cap) return 0;
//     do { byte = v & 0x7F; v >>= 7; if (v != 0) byte |= 0x80; out[n++] = byte; } while (v);
//
//   decode_varint(buf, len, out):
//     for i in 0..len-1:
//       if (i >= 10) return false;                       // too long
//       byte = buf[i];
//       if (i == 9) {                                    // 10th byte
//         if ((byte & 0xFE) != 0) return false;          // must be <= 0x01
//         *out = result | (byte & 0x01) << 63;
//         return true;
//       }
//       result |= (byte & 0x7F) << (7 * i);
//       if ((byte & 0x80) == 0) { *out = result; return true; }
//     return false;   // truncated
//
//   encode_frame: 4 bytes LE length, then varint(uid), then payload bytes.
//   decode_frame: len < 4 -> nullopt; parse uid varint (must succeed); payload
//     must fit in the remaining bytes -> nullopt otherwise.

namespace codec {

std::size_t encode_varint(std::uint64_t v, std::byte* out, std::size_t cap) {
  (void)v;
  (void)out;
  (void)cap;
  return 0;
}

bool decode_varint(const std::byte* buf, std::size_t len, std::uint64_t& out) {
  (void)buf;
  (void)len;
  (void)out;
  return false;
}

std::size_t varint_len(std::uint64_t /*v*/) { return 0; }

std::size_t encode_frame(const Message& /*m*/, std::byte* /*out*/,
                         std::size_t /*cap*/) {
  return 0;
}

std::size_t frame_size(const Message& /*m*/) { return 0; }

std::optional<Message> decode_frame(const std::byte* /*buf*/,
                                    std::size_t /*len*/) {
  return std::nullopt;
}

}  // namespace codec