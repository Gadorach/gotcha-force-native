#pragma once
// Portable scalar and byte-order layer for the native rewrite.
//
// Rules for this tree:
//   * No SDL, Vulkan, OS headers or emulator headers in engine/core.
//   * Fixed-width integers everywhere; `int` is never used for game data.
//   * Guest-derived values keep their original 32-bit width. A "guest pointer"
//     is a u32 address, never a host pointer.
//   * Multi-byte game data is big-endian. Convert explicitly at the boundary;
//     do not rely on host byte order or on packed struct layout.
#include <bit>
#include <cstddef>
#include <cstdint>

namespace gf {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using f32 = float;
using f64 = double;

// A 32-bit guest address. Never dereference directly; resolve through the
// address space of whatever subsystem owns it.
struct GuestAddress {
    u32 value = 0;
    constexpr explicit operator bool() const { return value != 0; }
    friend constexpr bool operator==(GuestAddress a, GuestAddress b) { return a.value == b.value; }
};

// Host float bit patterns and guest float bit patterns are both IEEE-754
// binary32, but endianness differs, so all conversion is explicit.
constexpr u32 load_be32(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}
constexpr u16 load_be16(const u8* p) { return u16((u16(p[0]) << 8) | u16(p[1])); }
constexpr void store_be32(u8* p, u32 v) {
    p[0] = u8(v >> 24); p[1] = u8(v >> 16); p[2] = u8(v >> 8); p[3] = u8(v);
}
constexpr void store_be16(u8* p, u16 v) { p[0] = u8(v >> 8); p[1] = u8(v); }

constexpr f32 bits_to_f32(u32 bits) { return std::bit_cast<f32>(bits); }
constexpr u32 f32_to_bits(f32 value) { return std::bit_cast<u32>(value); }
constexpr f32 load_be_f32(const u8* p) { return bits_to_f32(load_be32(p)); }
constexpr void store_be_f32(u8* p, f32 value) { store_be32(p, f32_to_bits(value)); }

// Read-only window over a guest buffer. Bounds are checked; callers that
// already validated a range can use the unchecked forms.
struct ByteView {
    const u8* data = nullptr;
    std::size_t size = 0;
    constexpr bool contains(std::size_t offset, std::size_t bytes) const {
        return data != nullptr && offset <= size && bytes <= size - offset;
    }
    constexpr u32 be32(std::size_t offset) const { return load_be32(data + offset); }
    constexpr u16 be16(std::size_t offset) const { return load_be16(data + offset); }
    constexpr f32 bef32(std::size_t offset) const { return load_be_f32(data + offset); }
};

struct MutableByteView {
    u8* data = nullptr;
    std::size_t size = 0;
    constexpr bool contains(std::size_t offset, std::size_t bytes) const {
        return data != nullptr && offset <= size && bytes <= size - offset;
    }
    constexpr void set_be32(std::size_t offset, u32 value) const { store_be32(data + offset, value); }
    constexpr void set_be16(std::size_t offset, u16 value) const { store_be16(data + offset, value); }
    constexpr void set_bef32(std::size_t offset, f32 value) const { store_be_f32(data + offset, value); }
};

} // namespace gf
