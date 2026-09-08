# Exercise containers/05_varint_codec (ex32) — Varint Message Codec (Task)

## The problem (in plain words)

Real systems don't pass structs over a wire — they pass **bytes**. This
exercise builds the workhorse encoding of any binary protocol: the **varint**
(Protobuf's LEB128: a number squeezed into as few bytes as the magnitude
needs) and a **framed message** built on top of it:

```
[ 4-byte LE payload length ][ varint client_uid ][ payload bytes ... ]
```

Master the compression (7 bits per byte, high bit = "more to come"), the
boundary cases (a `uint64` needs at most 10 bytes; the 10th byte can only
be 0 or 1), and the discipline that decode never crashes on garbage — it
returns `false` / `nullopt` instead.

## Requirements (what the tests check)

1. **varint encoding:** `encode_varint` produces the exact known byte patterns
   (`0 → 0x00`, `127 → 0x7F`, `128 → 0x80 0x01`, `300 → 0xAC 0x02`,
   `UINT64_MAX → 9×0xFF + 0x01`), writing 1–10 bytes.
2. **Buffer safety:** if the output buffer is too small, `encode_varint`
   returns 0 **without writing a single byte**.
3. **varint decoding:** `decode_varint` reads those encodings back, stops at
   the terminator (trailing bytes are ignored) and:
   - returns `false` on **truncated** input (buffer ends before a terminator);
   - returns `false` on **overflow** (more than 10 bytes, or a 10th byte > 1);
   - leaves `out` untouched on failure.
4. **`frame_size`:** exactly `4 + varint_len(uid) + payload.size()`.
5. **Framing round-trip:** `encode_frame` → `decode_frame` recovers
   `{client_uid, payload}` exactly, for uid 300 + arbitrary payloads, and for a
   hundred randomized cases.
6. **Frame boundary discipline:** truncated frames, frames shorter than the
   4-byte header, and a header claiming more payload than the buffer holds
   all decode to `nullopt` — never a crash, never a partial read.
7. `encode_frame` also returns 0 on too-small buffers (nothing written).

## Public API

```cpp
namespace codec {

struct Message { std::uint64_t client_uid; std::vector<std::byte> payload; };

std::size_t encode_varint(std::uint64_t v, std::byte* out, std::size_t cap);  // bytes written, 0 = too small
bool   decode_varint(const std::byte* buf, std::size_t len, std::uint64_t& out);
std::size_t varint_len(std::uint64_t v);                                       // 1..10
std::size_t encode_frame(const Message& m, std::byte* out, std::size_t cap);
std::size_t frame_size(const Message& m);
std::optional<Message> decode_frame(const std::byte* buf, std::size_t len);

}  // namespace codec
```

## How to think about it (suggested design)

- **Encode loop:** `do { byte = v & 0x7F; v >>= 7; if (v) byte |= 0x80; } while (v);`
  — the `do/while` guarantees at least one byte even for zero, and the `0x80`
  bit marks "another byte follows". Precompute `varint_len` first so you can
  reject a too-small buffer up front.
- **Decode loop:** walk the buffer, accumulating 7-bit groups into `result`
  with increasing shifts (`shift += 7`). Stop as soon as a byte has its MSB
  clear — that's the terminator. Failure rules:
  - reached the end of the buffer without a terminator → truncated;
  - went past 10 bytes / hit the 10th byte with value > 1 → overflow.
  Both return `false`. This is where most interview bugs live; the tests
  hammer exactly these.
- **Frame:** write the 4 header bytes (little-endian payload length), then the
  uid **varint**, then copy the payload. `decode_frame` is the reverse with all
  the same failure returns (`nullopt`), plus: header length + uid length must
  not reach past the buffer.
- Style choice you should notice: **no exceptions** for malformed data — return
  values/`optional` are the contract of a wire decoder (you never trust bytes).

## Make it harder (optional — not covered by the tests)

- **`decode_frame_exact`:** a validator that requires the ENTIRE buffer to be
  consumed by exactly one frame (no trailing slack) — the strict-mode variant
  a stateless receiver uses.
- **Byte-stream walker:** `next_frame(buf, len)` that peeks the length,
  decodes one frame, and returns the leftover as a view — enabling the
  `std::vector<std::byte>` reassembly loop (or a `pmr` flat buffer).
- **Append-aware copy:** encode straight into a growing
  `std::vector<std::byte>` instead of a fixed buffer, avoiding the
  two-step size-then-copy dance.
- **`decode_varint` with consumption report:** return how many bytes were used,
  so a reader can seek past the varint without re-scanning (useful for the
  next exercise).
- **Byte-level differential tests:** fuzz `decode_varint` / `decode_frame`
  with random byte strings; assert you never read out of bounds (ASan build).
  A property check that `decode(encode(x)) == x` over millions of values.

## Files

- Stub: `src/varint_codec.cpp`
- Tests: `test/test_varint_codec.cpp`
- Reference: `SOLUTION.md`