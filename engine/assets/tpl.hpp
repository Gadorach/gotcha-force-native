#pragma once
// TPL texture decoder for GX (GameCube/Wii) texture images.
//
// TPL files wrap one or more raw GX textures. Each image carries a 36-byte
// header (height, width, format, pixel offset, then GX sampler state) and,
// for the indexed formats, a pointer to a palette header. All fields are
// big-endian; the file itself is never interpreted as a host struct.
//
// Supported GX formats (the set the game actually ships):
//   0  I4      8x8 blocks, 4-bit intensity nibbles
//   2  IA4     8x4 blocks, 4-bit intensity + 4-bit alpha per pixel
//   4  RGB565  4x4 blocks, one 16-bit word per pixel
//   5  RGB5A3  4x4 blocks, one 16-bit word per pixel (three-bit alpha branch)
//   6  RGBA8   4x4 blocks, 64 bytes: an AR plane then a GB plane
//   8  C4      8x8 blocks, 4-bit palette indices
//   9  C8      8x4 blocks, 8-bit palette indices
// Palette formats: 0 = IA8, 1 = RGB565, 2 = RGB5A3.
//
// Channel expansion is GX bit replication (repeat the packed bits until eight
// are available, then shift down), not floor(value * 255 / max). Intensity in
// I4 expands to all four channels, alpha included; IA4 keeps intensity in the
// low nibble and alpha in the high nibble, as the hardware and the reference
// decoders do.
//
// The decoder takes a plain byte window: no file I/O, no OS or GPU types, no
// globals, no exceptions. Every range is bounds-checked before a read, the
// header, image table, palette and pixel payloads must not overlap, and a
// rejected image clears the output instead of leaving half-decoded pixels.
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "engine/assets/gx_texture.hpp"
#include "engine/core/types.hpp"

namespace gf {

inline constexpr u32 kTplMagic = 0x0020AF30u;
inline constexpr u32 kTplMaxDimension = 4096;
inline constexpr u32 kTplMaxPaletteEntries = 256;
inline constexpr std::size_t kTplHeaderBytes = 12;           // magic, count, table offset
inline constexpr std::size_t kTplTableEntryBytes = 8;        // image header, palette header
inline constexpr std::size_t kTplImageHeaderBytes = 36;      // reference lower bound
inline constexpr std::size_t kTplPaletteHeaderBytes = 12;    // entries, flags, format, offset

// One decoded image: tightly packed RGBA8, top row first.
struct TplImage {
    u32 width = 0;
    u32 height = 0;
    u32 format = 0;              // GX texture format from the header
    u32 image_index = 0;
    u32 image_count = 0;
    bool indexed = false;
    u32 palette_format = 0;      // 0 = IA8, 1 = RGB565, 2 = RGB5A3
    std::vector<u8> rgba;        // width*height*4, RGBA8, top row first
};

namespace detail {

inline bool tpl_error(std::string* error, std::string message) {
    if (error) *error = std::move(message);
    return false;
}

// Bounds test in u64: an attacker-supplied offset must not wrap a size_t
// subtraction and pass the check.
inline bool tpl_has(const ByteView& file, u64 offset, u64 size) {
    return file.data != nullptr && offset <= u64(file.size) && size <= u64(file.size) - offset;
}

// GX channel expansion by bit replication: repeat `bits` until at least eight
// bits are available, then keep the top eight.
constexpr u8 tpl_expand(u32 value, u32 bits) {
    u32 result = value;
    u32 width = bits;
    while (width < 8) {
        result = (result << bits) | value;
        width += bits;
    }
    return u8(result >> (width - 8));
}

struct TplColor {
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 0;
};

// RGB565 (format 4) or RGB5A3 (format 5). RGB5A3 clears bit 15 for the
// three-bit-alpha branch: 4/4/4 colour plus a 3-bit alpha, with the alpha bits
// above the colour bits.
constexpr TplColor tpl_packed_color(u32 word, u32 format) {
    if (format == 4) {
        return {tpl_expand(word >> 11, 5), tpl_expand((word >> 5) & 63u, 6),
                tpl_expand(word & 31u, 5), 255};
    }
    if ((word & 0x8000u) != 0) {
        return {tpl_expand((word >> 10) & 31u, 5), tpl_expand((word >> 5) & 31u, 5),
                tpl_expand(word & 31u, 5), 255};
    }
    return {tpl_expand((word >> 8) & 15u, 4), tpl_expand((word >> 4) & 15u, 4),
            tpl_expand(word & 15u, 4), tpl_expand((word >> 12) & 7u, 3)};
}

struct TplRange {
    u64 start = 0;
    u64 size = 0;
    const char* name = "";
};

struct TplLayout {
    u32 width = 0;
    u32 height = 0;
    u32 format = 0;
    u32 image_index = 0;
    u32 image_count = 0;
    u32 palette_format = 0;
    u64 pixel_offset = 0;
    u64 pixel_bytes = 0;
    u64 palette_offset = 0;
    u64 palette_bytes = 0;
    std::vector<TplColor> palette;
};

// Validates the whole container before any pixel is produced. Mirrors the
// reference decoder: magic, table bounds, per-image header bounds, dimension
// cap, format whitelist, palette presence/format/count, and pairwise overlap of
// header, table, palette and payload ranges.
inline bool tpl_validate(const ByteView& file, u32 image_index, TplLayout* layout,
                         std::string* error) {
    if (!tpl_has(file, 0, kTplHeaderBytes)) {
        return tpl_error(error, "tpl: file is shorter than the 12-byte TPL header");
    }
    if (file.be32(0) != kTplMagic) {
        return tpl_error(error, "tpl: not a TPL file (bad magic, expected 0x0020AF30)");
    }
    const u32 count = file.be32(4);
    const u64 table = file.be32(8);
    if (count == 0) {
        return tpl_error(error, "tpl: image table declares no images");
    }
    const u64 table_bytes = u64(count) * kTplTableEntryBytes;
    if (!tpl_has(file, table, table_bytes)) {
        return tpl_error(error, "tpl: image table runs past the end of the file");
    }
    if (u64(image_index) >= u64(count)) {
        return tpl_error(error, "tpl: image index " + std::to_string(image_index) +
                                   " outside the declared image count " + std::to_string(count));
    }

    const u64 entry = table + u64(image_index) * kTplTableEntryBytes;
    const u64 image = file.be32(std::size_t(entry));
    const u64 palette_header = file.be32(std::size_t(entry) + 4);
    if (!tpl_has(file, image, 12)) {
        return tpl_error(error, "tpl: image header runs past the end of the file");
    }
    const u32 height = file.be16(std::size_t(image));
    const u32 width = file.be16(std::size_t(image) + 2);
    const u32 format = file.be32(std::size_t(image) + 4);
    const u64 pixel_offset = file.be32(std::size_t(image) + 8);

    if (width == 0 || height == 0) {
        return tpl_error(error, "tpl: zero width or height");
    }
    if (width > kTplMaxDimension || height > kTplMaxDimension) {
        return tpl_error(error, "tpl: dimensions " + std::to_string(width) + "x" +
                                   std::to_string(height) + " exceed the 4096 cap");
    }
    const bool indexed = format == 8 || format == 9;
    if (!indexed && format != 0 && format != 2 && format != 4 && format != 5 && format != 6) {
        return tpl_error(error, "tpl: unsupported GX texture format " + std::to_string(format));
    }

    // Block geometry: 16-bit formats and RGBA8 tile 4x4; I4 and C4 tile 8x8;
    // IA4 and C8 tile 8x4.
    u32 block_width = 4;
    u32 block_height = 4;
    u64 tile_bytes = 32;
    if (format == 6) {
        tile_bytes = 64;
    } else if (format == 0 || format == 8) {
        block_width = 8;
        block_height = 8;
    } else if (format == 2 || format == 9) {
        block_width = 8;
        block_height = 4;
    }
    const u64 pixel_bytes = u64((width + block_width - 1) / block_width) *
                            u64((height + block_height - 1) / block_height) * tile_bytes;

    TplRange ranges[6];
    std::size_t range_count = 0;
    ranges[range_count++] = {0, kTplHeaderBytes, "header"};
    ranges[range_count++] = {table, table_bytes, "image table"};
    ranges[range_count++] = {image, kTplImageHeaderBytes, "image header"};
    ranges[range_count++] = {pixel_offset, pixel_bytes, "pixel payload"};

    u32 palette_format = 0;
    u32 entries = 0;
    u64 palette_offset = 0;
    u64 palette_bytes = 0;
    if (indexed) {
        if (palette_header == 0) {
            return tpl_error(error, "tpl: indexed texture is missing its palette");
        }
        if (!tpl_has(file, palette_header, kTplPaletteHeaderBytes)) {
            return tpl_error(error, "tpl: palette header runs past the end of the file");
        }
        entries = file.be16(std::size_t(palette_header));
        // The unpacked/padding bytes at +2/+3 carry no decode meaning.
        palette_format = file.be32(std::size_t(palette_header) + 4);
        palette_offset = file.be32(std::size_t(palette_header) + 8);
        if (entries == 0 || entries > kTplMaxPaletteEntries) {
            return tpl_error(error, "tpl: palette entry count " + std::to_string(entries) +
                                       " outside 1..256");
        }
        if (palette_format > 2) {
            return tpl_error(error, "tpl: unsupported palette format " +
                                       std::to_string(palette_format) +
                                       " (only IA8, RGB565 and RGB5A3)");
        }
        palette_bytes = u64(entries) * 2;
        ranges[range_count++] = {palette_header, kTplPaletteHeaderBytes, "palette header"};
        ranges[range_count++] = {palette_offset, palette_bytes, "palette data"};
    } else if (palette_header != 0) {
        return tpl_error(error, "tpl: unexpected palette on a non-indexed texture");
    }

    for (std::size_t i = 0; i < range_count; ++i) {
        const TplRange& range = ranges[i];
        if (!tpl_has(file, range.start, range.size)) {
            return tpl_error(error, std::string("tpl: ") + range.name +
                                       " runs past the end of the file");
        }
        for (std::size_t j = 0; j < i; ++j) {
            const TplRange& other = ranges[j];
            if (range.start < other.start + other.size && other.start < range.start + range.size) {
                return tpl_error(error, "tpl: overlapping header/table/palette/payload ranges");
            }
        }
    }

    std::vector<TplColor> palette;
    if (indexed) {
        palette.resize(entries);
        for (u32 i = 0; i < entries; ++i) {
            const u16 word = file.be16(std::size_t(palette_offset) + std::size_t(i) * 2);
            if (palette_format == 0) {
                palette[i] = {u8(word), u8(word), u8(word), u8(word >> 8)};
            } else {
                palette[i] = tpl_packed_color(word, palette_format == 1 ? 4 : 5);
            }
        }
    }

    layout->width = width;
    layout->height = height;
    layout->format = format;
    layout->image_index = image_index;
    layout->image_count = count;
    layout->palette_format = palette_format;
    layout->pixel_offset = pixel_offset;
    layout->pixel_bytes = pixel_bytes;
    layout->palette_offset = palette_offset;
    layout->palette_bytes = palette_bytes;
    layout->palette = std::move(palette);
    return true;
}

} // namespace detail

// Reads the image table and reports how many images the file declares.
inline bool tpl_image_count(const ByteView& file, u32* count, std::string* error) {
    if (error) error->clear();
    if (!detail::tpl_has(file, 0, kTplHeaderBytes)) {
        return detail::tpl_error(error, "tpl: file is shorter than the 12-byte TPL header");
    }
    if (file.be32(0) != kTplMagic) {
        return detail::tpl_error(error, "tpl: not a TPL file (bad magic, expected 0x0020AF30)");
    }
    const u32 declared = file.be32(4);
    if (declared == 0) {
        return detail::tpl_error(error, "tpl: image table declares no images");
    }
    if (!detail::tpl_has(file, file.be32(8), u64(declared) * kTplTableEntryBytes)) {
        return detail::tpl_error(error, "tpl: image table runs past the end of the file");
    }
    if (count) *count = declared;
    return true;
}

// Decodes one image. The index is always explicit: a multi-image file never
// silently picks an image. Dimensions are capped at 4096x4096 by validation.
inline bool tpl_decode(const ByteView& file, u32 image_index, TplImage* out,
                       std::string* error) {
    if (error) error->clear();
    if (out == nullptr) return detail::tpl_error(error, "tpl: null output image");
    *out = TplImage{};

    detail::TplLayout layout;
    if (!detail::tpl_validate(file, image_index, &layout, error)) return false;

    TplImage image;
    image.width = layout.width;
    image.height = layout.height;
    image.format = layout.format;
    image.image_index = layout.image_index;
    image.image_count = layout.image_count;
    image.indexed = layout.format == 8 || layout.format == 9;
    image.palette_format = layout.palette_format;

    // Pixels come from the shared GX decoder, so TPL and HSD cannot drift apart.
    const ByteView pixels{file.data + std::size_t(layout.pixel_offset),
                          std::size_t(layout.pixel_bytes)};
    const ByteView palette =
        layout.palette_bytes != 0
            ? ByteView{file.data + std::size_t(layout.palette_offset),
                       std::size_t(layout.palette_bytes)}
            : ByteView{};
    std::string gx_error;
    if (!gx_decode(pixels, layout.width, layout.height, layout.format, palette,
                   layout.palette_format, static_cast<u32>(layout.palette.size()), &image.rgba,
                   &gx_error)) {
        if (error) *error = "tpl: " + gx_error;
        return false;
    }

    *out = std::move(image);
    return true;
}

} // namespace gf
