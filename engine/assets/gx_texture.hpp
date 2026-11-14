#pragma once
// Shared GX texture decode for the rewrite.
//
// TPL files (whole texture files) and HSD archives (textures described by an
// ImageDesc pointing at raw GX data) carry the same encodings, so the tiling and
// channel rules live here once. Both decoders call `gx_decode`; the TPL layer
// keeps its own container validation, and HSD supplies an explicit palette.
//
// Formats (GX TEV texture formats):
//   0  I4      8x8 tiles, 4-bit intensity (nibble, high nibble first)
//   1  I8      8x4 tiles, 8-bit intensity
//   2  IA4     8x4 tiles, intensity low nibble, alpha high nibble
//   3  IA8     4x4 tiles, 16-bit: intensity low byte, alpha high byte
//   4  RGB565  4x4 tiles, 16-bit words
//   5  RGB5A3  4x4 tiles, 16-bit words (bit 15 clear = 4/4/4 + 3-bit alpha)
//   6  RGBA8   4x4 tiles, 64 bytes: an AR plane then a GB plane
//   8  C4      8x8 tiles, 4-bit palette indices
//   9  C8      8x4 tiles, 8-bit palette indices
//   14 CMPR    8x8 tiles, 4 x 4x4 sub-blocks with two RGB565 endpoints
//
// Note the numbering: CMPR is GX_TF_CMPR = 0xE = 14. Format 10 is C14X2, which
// this game does not ship, so it is refused rather than decoded as CMPR.
//
// Channel expansion is GX bit replication, never `value * 255 / max`. Intensity
// formats expand into all four channels, alpha included, which is why cut-out
// art written to I4/I8 needs an alpha test rather than blending.
#include <cstddef>
#include <string>
#include <vector>

#include "engine/core/types.hpp"

namespace gf {

inline constexpr u32 kGxMaxDimension = 4096;

// GX channel expansion by bit replication: repeat the bits until eight are
// available, then keep the top eight.
constexpr u8 gx_expand(u32 value, u32 bits) {
    u32 result = value;
    u32 width = bits;
    while (width < 8) {
        result = (result << bits) | value;
        width += bits;
    }
    return u8(result >> (width - 8));
}

struct GxColor {
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 255;
};

// RGB565 (format 4) or RGB5A3 (format 5).
constexpr GxColor gx_packed_color(u32 word, u32 format) {
    if (format == 4) {
        return {gx_expand(word >> 11, 5), gx_expand((word >> 5) & 63u, 6),
                gx_expand(word & 31u, 5), 255};
    }
    if ((word & 0x8000u) != 0) {
        return {gx_expand((word >> 10) & 31u, 5), gx_expand((word >> 5) & 31u, 5),
                gx_expand(word & 31u, 5), 255};
    }
    return {gx_expand((word >> 8) & 15u, 4), gx_expand((word >> 4) & 15u, 4),
            gx_expand(word & 15u, 4), gx_expand((word >> 12) & 7u, 3)};
}

constexpr GxColor gx_palette_color(u32 word, u32 palette_format) {
    if (palette_format == 0) {                       // IA8
        const u8 intensity = u8(word & 0xFFu);
        return {intensity, intensity, intensity, u8(word >> 8)};
    }
    return gx_packed_color(word, palette_format == 1 ? 4u : 5u);
}

// Tile geometry and payload size for one image. Kept public so callers can
// bound a buffer before decoding (HSD reads payloads out of a larger archive).
inline bool gx_texture_size(u32 width, u32 height, u32 format, u64* bytes,
                            std::string* error) {
    if (width == 0 || height == 0) {
        if (error) *error = "gx: zero width or height";
        return false;
    }
    if (width > kGxMaxDimension || height > kGxMaxDimension) {
        if (error) *error = "gx: dimensions exceed the 4096 cap";
        return false;
    }
    u32 block_width = 4;
    u32 block_height = 4;
    u64 tile_bytes = 32;
    switch (format) {
    case 0: case 8: block_width = 8; block_height = 8; break;
    case 1: case 2: case 9: block_width = 8; block_height = 4; break;
    case 3: case 4: case 5: block_width = 4; block_height = 4; break;
    case 6: block_width = 4; block_height = 4; tile_bytes = 64; break;
    case 14: block_width = 8; block_height = 8; break;
    default:
        if (error) *error = "gx: unsupported texture format " + std::to_string(format);
        return false;
    }
    if (bytes) {
        *bytes = u64((width + block_width - 1) / block_width) *
                 u64((height + block_height - 1) / block_height) * tile_bytes;
    }
    return true;
}

namespace detail {

inline void gx_store(std::vector<u8>& rgba, u32 width, u32 x, u32 y, GxColor colour) {
    const std::size_t at = (std::size_t(y) * width + x) * 4;
    rgba[at + 0] = colour.r;
    rgba[at + 1] = colour.g;
    rgba[at + 2] = colour.b;
    rgba[at + 3] = colour.a;
}

inline u8 gx_byte(const ByteView& data, u64 at) { return data.data[at]; }

inline u32 gx_be16(const ByteView& data, u64 at) {
    return (u32(data.data[at]) << 8) | u32(data.data[at + 1]);
}

inline u32 gx_be32(const ByteView& data, u64 at) {
    return (u32(data.data[at]) << 24) | (u32(data.data[at + 1]) << 16) |
           (u32(data.data[at + 2]) << 8) | u32(data.data[at + 3]);
}

} // namespace detail

// Decodes one GX texture into tightly packed RGBA8, top row first. `palette` is
// required exactly for the indexed formats (8, 9) and must be empty otherwise.
// Every read is bounds-checked against the caller-provided window.
inline bool gx_decode(ByteView pixels, u32 width, u32 height, u32 format,
                      ByteView palette, u32 palette_format, u32 palette_entries,
                      std::vector<u8>* out, std::string* error) {
    if (!out) {
        if (error) *error = "gx: no output buffer";
        return false;
    }
    out->clear();
    u64 needed = 0;
    if (!gx_texture_size(width, height, format, &needed, error)) return false;
    if (!pixels.data || pixels.size < needed) {
        if (error) *error = "gx: pixel payload is shorter than the declared image";
        return false;
    }
    const bool indexed = format == 8 || format == 9;
    if (indexed) {
        if (!palette.data || palette_entries == 0 || palette_entries > 256) {
            if (error) *error = "gx: indexed texture needs a palette of 1..256 entries";
            return false;
        }
        if (palette_format > 2) {
            if (error) *error = "gx: unsupported palette format " + std::to_string(palette_format);
            return false;
        }
        if (u64(palette.size) < u64(palette_entries) * 2) {
            if (error) *error = "gx: palette payload is shorter than the declared entries";
            return false;
        }
    } else if (palette.data) {
        if (error) *error = "gx: unexpected palette on a non-indexed texture";
        return false;
    }

    out->assign(std::size_t(width) * height * 4, 0);

    switch (format) {
    case 3: {   // IA8, 4x4 tiles
        u64 at = 0;
        for (u32 ty = 0; ty < height; ty += 4) {
            for (u32 tx = 0; tx < width; tx += 4) {
                for (u32 y = 0; y < 4; ++y) {
                    for (u32 x = 0; x < 4; ++x) {
                        const u32 word = detail::gx_be16(pixels, at);
                        at += 2;
                        if (tx + x >= width || ty + y >= height) continue;
                        const u8 intensity = u8(word & 0xFFu);
                        detail::gx_store(*out, width, tx + x, ty + y,
                                         GxColor{intensity, intensity, intensity, u8(word >> 8)});
                    }
                }
            }
        }
        return true;
    }
    case 4:
    case 5: {   // RGB565 / RGB5A3, 4x4 tiles
        u64 at = 0;
        for (u32 ty = 0; ty < height; ty += 4) {
            for (u32 tx = 0; tx < width; tx += 4) {
                for (u32 y = 0; y < 4; ++y) {
                    for (u32 x = 0; x < 4; ++x) {
                        const u32 word = detail::gx_be16(pixels, at);
                        at += 2;
                        if (tx + x >= width || ty + y >= height) continue;
                        detail::gx_store(*out, width, tx + x, ty + y,
                                         gx_packed_color(word, format));
                    }
                }
            }
        }
        return true;
    }
    case 6: {   // RGBA8: per tile an AR plane then a GB plane
        u64 tile = 0;
        for (u32 ty = 0; ty < height; ty += 4) {
            for (u32 tx = 0; tx < width; tx += 4) {
                for (u32 y = 0; y < 4; ++y) {
                    for (u32 x = 0; x < 4; ++x) {
                        const u64 p = tile + (u64(y) * 4 + x) * 2;
                        const u8 a = detail::gx_byte(pixels, p);
                        const u8 r = detail::gx_byte(pixels, p + 1);
                        const u8 g = detail::gx_byte(pixels, p + 32);
                        const u8 b = detail::gx_byte(pixels, p + 33);
                        if (tx + x >= width || ty + y >= height) continue;
                        detail::gx_store(*out, width, tx + x, ty + y, GxColor{r, g, b, a});
                    }
                }
                tile += 64;
            }
        }
        return true;
    }
    case 14: {  // CMPR: 8x8 blocks of four 4x4 sub-blocks
        u64 at = 0;
        for (u32 by = 0; by < height; by += 8) {
            for (u32 bx = 0; bx < width; bx += 8) {
                for (u32 sub = 0; sub < 4; ++sub) {
                    const u32 c0 = detail::gx_be16(pixels, at);
                    const u32 c1 = detail::gx_be16(pixels, at + 2);
                    at += 4;
                    const GxColor a = gx_packed_color(c0, 4);
                    const GxColor b = gx_packed_color(c1, 4);
                    GxColor colors[4];
                    colors[0] = a;
                    colors[1] = b;
                    // Dolphin's DXTBlend(v1, v2) = (3 * v1 + 5 * v2) >> 3, which
                    // is the blend the project's texture oracle uses; the second
                    // endpoint is passed first so colour 2 leans towards c0.
                    const auto blend = [](u32 v1, u32 v2) { return u8((v1 * 3u + v2 * 5u) >> 3); };
                    if (c0 > c1) {
                        colors[2] = GxColor{blend(b.r, a.r), blend(b.g, a.g), blend(b.b, a.b), 255};
                        colors[3] = GxColor{blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), 255};
                    } else {
                        colors[2] = GxColor{u8((a.r + b.r) / 2u), u8((a.g + b.g) / 2u),
                                            u8((a.b + b.b) / 2u), 255};
                        // The fourth slot of the two-colour block is transparent;
                        // the RGB is kept, matching the reference decoder.
                        colors[3] = GxColor{u8((a.r + b.r) / 2u), u8((a.g + b.g) / 2u),
                                            u8((a.b + b.b) / 2u), 0};
                    }
                    const u32 bits = detail::gx_be32(pixels, at);
                    at += 4;
                    const u32 sx = (sub % 2) * 4;
                    const u32 sy = (sub / 2) * 4;
                    for (u32 y = 0; y < 4; ++y) {
                        for (u32 x = 0; x < 4; ++x) {
                            const u32 index = (bits >> (2 * (15 - (y * 4 + x)))) & 3u;
                            const u32 px = bx + sx + x;
                            const u32 py = by + sy + y;
                            if (px >= width || py >= height) continue;
                            detail::gx_store(*out, width, px, py, colors[index]);
                        }
                    }
                }
            }
        }
        return true;
    }
    default:
        break;
    }

    // Intensity, IA and indexed formats share the tile walk.
    // I8, IA4 and C8 are 8x4 tiles; I4 and C4 are 8x8. Getting this wrong reads
    // twice the payload (I8 was doing exactly that until ASan caught it).
    const u32 block_height = (format == 1 || format == 2 || format == 9) ? 4 : 8;
    const bool nibbles = (format == 0 || format == 8);
    u64 at = 0;
    for (u32 ty = 0; ty < height; ty += block_height) {
        for (u32 tx = 0; tx < width; tx += 8) {
            for (u32 y = 0; y < block_height; ++y) {
                for (u32 x = 0; x < 8; ++x) {
                    u32 index = 0;
                    if (nibbles) {
                        index = (x % 2 == 0) ? (u32(detail::gx_byte(pixels, at)) >> 4)
                                             : (u32(detail::gx_byte(pixels, at)) & 15u);
                        if (x % 2 == 1) at += 1;
                    } else {
                        index = detail::gx_byte(pixels, at);
                        at += 1;
                    }
                    if (tx + x >= width || ty + y >= height) continue;
                    GxColor colour{};
                    if (format == 0) {          // I4
                        const u8 value = gx_expand(index, 4);
                        colour = GxColor{value, value, value, value};
                    } else if (format == 1) {   // I8
                        colour = GxColor{u8(index), u8(index), u8(index), u8(index)};
                    } else if (format == 2) {   // IA4
                        const u8 intensity = gx_expand(index & 15u, 4);
                        colour = GxColor{intensity, intensity, intensity, gx_expand(index >> 4, 4)};
                    } else {                    // C4 (8) / C8 (9)
                        if (index >= palette_entries) {
                            out->clear();
                            if (error) *error = "gx: pixel index outside the declared palette";
                            return false;
                        }
                        colour = gx_palette_color(detail::gx_be16(palette, u64(index) * 2),
                                                  palette_format);
                    }
                    detail::gx_store(*out, width, tx + x, ty + y, colour);
                }
            }
        }
    }
    return true;
}

} // namespace gf
