#pragma once
// Portable image container and a minimal TGA decode.
//
// The engine wants one canonical pixel layout at the boundary: tightly packed
// RGBA8 with a top-left origin. Source assets may be stored differently, so the
// decoder normalises them here instead of leaking format quirks into renderers.
//
// Supported input is deliberately narrow and fails loudly otherwise:
//   * TGA image type 2 (uncompressed true-colour) only.
//   * 24 or 32 bits per pixel, no colour map, no RLE.
//   * Either origin; bottom-left rows are flipped to top-left.
//
// Decoding takes a plain byte window: no file I/O, no OS or GPU types, no
// globals. Bounds are checked before any read, and a rejected image leaves the
// output empty.
#include <cstddef>
#include <string>
#include <vector>

#include "engine/core/types.hpp"

namespace gf {

inline constexpr std::size_t kTgaHeaderBytes = 18;
inline constexpr u8 kTgaTypeTrueColour = 2;
inline constexpr u8 kTgaTopLeftOrigin = 0x20;  // descriptor bit 5
inline constexpr u32 kMaxImageDimension = 8192;
inline constexpr std::size_t kMaxImagePixels = std::size_t(kMaxImageDimension) * kMaxImageDimension;

// Tightly packed RGBA8, top-left origin, row major.
struct Image {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> rgba;
};

namespace detail {

inline bool texture_error(std::string* error, std::string message) {
    if (error) *error = message;
    return false;
}

} // namespace detail

inline bool decode_tga(const ByteView& buffer, Image& out, std::string* error) {
    out = Image{};
    if (error) error->clear();

    if (!buffer.contains(0, kTgaHeaderBytes)) {
        return detail::texture_error(error, "texture: buffer shorter than the 18-byte TGA header");
    }

    const u8 id_length = buffer.data[0];
    const u8 colour_map_type = buffer.data[1];
    const u8 image_type = buffer.data[2];
    const u16 colour_map_length = buffer.be16(5);
    const u8 colour_map_depth = buffer.data[7];
    const u32 width = buffer.be16(12);
    const u32 height = buffer.be16(14);
    const u8 bpp = buffer.data[16];
    const u8 descriptor = buffer.data[17];

    if (colour_map_type != 0 || colour_map_length != 0 || colour_map_depth != 0) {
        return detail::texture_error(error, "texture: colour-mapped TGA is not supported");
    }
    if (image_type != kTgaTypeTrueColour) {
        return detail::texture_error(
            error, "texture: unsupported TGA image type " + std::to_string(image_type) +
                       " (only uncompressed true-colour, type 2, is accepted)");
    }
    if (bpp != 24 && bpp != 32) {
        return detail::texture_error(error,
                                     "texture: unsupported bit depth " + std::to_string(bpp) +
                                         " (only 24 or 32 bits per pixel)");
    }
    if (width == 0 || height == 0) {
        return detail::texture_error(error, "texture: zero width or height");
    }
    if (width > kMaxImageDimension || height > kMaxImageDimension) {
        return detail::texture_error(error,
                                     "texture: dimensions " + std::to_string(width) + "x" +
                                         std::to_string(height) + " exceed cap " +
                                         std::to_string(kMaxImageDimension));
    }

    const std::size_t bytes_per_pixel = bpp / 8;
    const std::size_t pixel_bytes = std::size_t(width) * std::size_t(height) * bytes_per_pixel;
    const std::size_t pixel_offset = kTgaHeaderBytes + std::size_t(id_length);
    if (!buffer.contains(pixel_offset, pixel_bytes)) {
        return detail::texture_error(
            error, "texture: truncated pixel data, need " + std::to_string(pixel_bytes) +
                       " bytes at offset " + std::to_string(pixel_offset) + ", have " +
                       std::to_string(pixel_offset <= buffer.size ? buffer.size - pixel_offset : 0));
    }

    const bool top_left = (descriptor & kTgaTopLeftOrigin) != 0;
    out.width = width;
    out.height = height;
    out.rgba.resize(std::size_t(width) * std::size_t(height) * 4);

    for (u32 y = 0; y < height; ++y) {
        const u32 source_y = top_left ? y : (height - 1u - y);
        for (u32 x = 0; x < width; ++x) {
            const std::size_t src =
                pixel_offset + (std::size_t(source_y) * width + x) * bytes_per_pixel;
            const std::size_t dst = (std::size_t(y) * width + x) * 4;
            out.rgba[dst + 0] = buffer.data[src + 2];  // R out of BGR(A)
            out.rgba[dst + 1] = buffer.data[src + 1];  // G
            out.rgba[dst + 2] = buffer.data[src + 0];  // B
            out.rgba[dst + 3] = bytes_per_pixel == 4 ? buffer.data[src + 3] : u8(255);
        }
    }
    return true;
}

// Serialise an image as a 32-bit, top-left-origin, uncompressed TGA. Used for
// fixtures and for writing canonical screenshots/dumps. Missing pixels in a
// short `rgba` buffer are written as zeroes rather than read out of bounds.
inline std::vector<u8> encode_tga(const Image& image) {
    const std::size_t pixels = std::size_t(image.width) * std::size_t(image.height);
    const std::size_t pixel_bytes = pixels * 4;
    std::vector<u8> bytes(kTgaHeaderBytes + pixel_bytes, 0);

    bytes[2] = kTgaTypeTrueColour;
    store_be16(bytes.data() + 12, u16(image.width));
    store_be16(bytes.data() + 14, u16(image.height));
    bytes[16] = 32;
    bytes[17] = u8(kTgaTopLeftOrigin | 0x08);  // top-left, 8 alpha bits

    const std::size_t available = image.rgba.size();
    for (std::size_t p = 0; p < pixels; ++p) {
        const std::size_t src = p * 4;
        const std::size_t dst = kTgaHeaderBytes + p * 4;
        const u8 r = src + 0 < available ? image.rgba[src + 0] : u8(0);
        const u8 g = src + 1 < available ? image.rgba[src + 1] : u8(0);
        const u8 b = src + 2 < available ? image.rgba[src + 2] : u8(0);
        const u8 a = src + 3 < available ? image.rgba[src + 3] : u8(0);
        bytes[dst + 0] = b;  // TGA stores blue first
        bytes[dst + 1] = g;
        bytes[dst + 2] = r;
        bytes[dst + 3] = a;
    }
    return bytes;
}

} // namespace gf
