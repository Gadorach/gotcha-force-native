// TPL decoder tests.
//
// Three groups: synthetic fixtures whose expected RGBA bytes are derived from
// the GX format rules by hand (bit-replication tables written out here, not
// taken from the decoder), one rejection case for every validation rule, and a
// read-only sweep of the real archive that decodes every image of every TPL
// member when the local game files happen to be present.
//
// Run from the project root; the real-archive group is skipped when
// original/extracted/files/afs_data.afs is absent.
#include "engine/assets/afs.hpp"
#include "engine/assets/tpl.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using gf::u16;
using gf::u32;
using gf::u8;

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "tpl failure: %s\n", what);
        std::abort();
    }
}

// GX channel expansion by bit replication, written out by hand:
//   n bits -> repeat the pattern until 8+ bits, keep the top 8.
constexpr u8 kRep3[8] = {0, 36, 73, 109, 146, 182, 219, 255};
constexpr u8 kRep4[16] = {0, 17, 34, 51, 68, 85, 102, 119,
                          136, 153, 170, 187, 204, 221, 238, 255};
constexpr u8 kRep5[32] = {0, 8, 16, 24, 33, 41, 49, 57, 66, 74, 82, 90, 99, 107, 115, 123,
                          132, 140, 148, 156, 165, 173, 181, 189, 198, 206, 214, 222, 231, 239,
                          247, 255};
constexpr u8 kRep6[64] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
                          65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125,
                          130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182,
                          186, 190, 195, 199, 203, 207, 211, 215, 219, 223, 227, 231, 235, 239,
                          243, 247, 251, 255};

// ---- synthetic TPL assembly ------------------------------------------

struct ImageSpec {
    u32 width = 0;
    u32 height = 0;
    u32 format = 0;
    std::vector<u8> pixels;
    std::vector<u16> palette;
    u32 palette_format = 0;
};

ImageSpec make_spec(u32 width, u32 height, u32 format, std::vector<u8> pixels) {
    ImageSpec spec;
    spec.width = width;
    spec.height = height;
    spec.format = format;
    spec.pixels = std::move(pixels);
    return spec;
}

void put16(std::vector<u8>& bytes, std::size_t at, u16 value) {
    bytes[at] = u8(value >> 8);
    bytes[at + 1] = u8(value & 0xFFu);
}

void put32(std::vector<u8>& bytes, std::size_t at, u32 value) {
    bytes[at] = u8(value >> 24);
    bytes[at + 1] = u8((value >> 16) & 0xFFu);
    bytes[at + 2] = u8((value >> 8) & 0xFFu);
    bytes[at + 3] = u8(value & 0xFFu);
}

struct Built {
    std::vector<u8> bytes;
    std::size_t table = 0;
    std::vector<std::size_t> image_header;
    std::vector<std::size_t> pixel_offset;
    std::vector<std::size_t> palette_header;  // 0 when the image declares none
};

// Header, table, image headers, then every pixel payload, then every palette
// header and its entries. Nothing overlaps, so a valid file only fails when a
// test deliberately corrupts it.
Built build(const std::vector<ImageSpec>& images) {
    Built built;
    const std::size_t count = images.size();
    std::size_t cursor = 12;
    built.table = cursor;
    cursor += count * 8;
    for (std::size_t i = 0; i < count; ++i) {
        built.image_header.push_back(cursor);
        cursor += 36;
    }
    for (std::size_t i = 0; i < count; ++i) {
        built.pixel_offset.push_back(cursor);
        cursor += images[i].pixels.size();
    }
    for (std::size_t i = 0; i < count; ++i) {
        if (images[i].palette.empty()) {
            built.palette_header.push_back(0);
        } else {
            built.palette_header.push_back(cursor);
            cursor += 12 + images[i].palette.size() * 2;
        }
    }

    built.bytes.assign(cursor, 0);
    std::vector<u8>& b = built.bytes;
    put32(b, 0, gf::kTplMagic);
    put32(b, 4, u32(count));
    put32(b, 8, u32(built.table));
    for (std::size_t i = 0; i < count; ++i) {
        put32(b, built.table + i * 8, u32(built.image_header[i]));
        put32(b, built.table + i * 8 + 4, u32(built.palette_header[i]));
        put16(b, built.image_header[i], u16(images[i].height));
        put16(b, built.image_header[i] + 2, u16(images[i].width));
        put32(b, built.image_header[i] + 4, images[i].format);
        put32(b, built.image_header[i] + 8, u32(built.pixel_offset[i]));
        for (std::size_t k = 0; k < images[i].pixels.size(); ++k) {
            b[built.pixel_offset[i] + k] = images[i].pixels[k];
        }
        if (built.palette_header[i] != 0) {
            const std::size_t header = built.palette_header[i];
            put16(b, header, u16(images[i].palette.size()));
            put32(b, header + 4, images[i].palette_format);
            put32(b, header + 8, u32(header + 12));
            for (std::size_t k = 0; k < images[i].palette.size(); ++k) {
                put16(b, header + 12 + k * 2, images[i].palette[k]);
            }
        }
    }
    return built;
}

gf::ByteView view(const std::vector<u8>& bytes) {
    return gf::ByteView{bytes.data(), bytes.size()};
}

// Raster-order payload for 16-bit tile formats: full 4x4 blocks, out-of-range
// words filled by clamping so the block layout stays complete.
template <typename WordFn>
std::vector<u8> pack16(u32 width, u32 height, WordFn word) {
    std::vector<u8> out;
    for (u32 ty = 0; ty < height; ty += 4) {
        for (u32 tx = 0; tx < width; tx += 4) {
            for (u32 y = 0; y < 4; ++y) {
                for (u32 x = 0; x < 4; ++x) {
                    const u32 px = tx + x < width ? tx + x : width - 1;
                    const u32 py = ty + y < height ? ty + y : height - 1;
                    const u16 value = word(px, py);
                    out.push_back(u8(value >> 8));
                    out.push_back(u8(value & 0xFFu));
                }
            }
        }
    }
    return out;
}

// Payload for indexed/intensity formats: 8x8 blocks (I4/C4) with two nibbles
// per byte, high nibble first, or 8x4 blocks (IA4/C8) with one byte per pixel.
template <typename IndexFn>
std::vector<u8> pack8(u32 width, u32 height, u32 format, IndexFn index) {
    const bool byte_per_pixel = format == 2 || format == 9;
    const u32 block_height = byte_per_pixel ? 4u : 8u;
    std::vector<u8> out;
    for (u32 ty = 0; ty < height; ty += block_height) {
        for (u32 tx = 0; tx < width; tx += 8) {
            for (u32 y = 0; y < block_height; ++y) {
                const u32 py = ty + y < height ? ty + y : height - 1;
                if (byte_per_pixel) {
                    for (u32 x = 0; x < 8; ++x) {
                        const u32 px = tx + x < width ? tx + x : width - 1;
                        out.push_back(u8(index(px, py) & 0xFFu));
                    }
                } else {
                    for (u32 x = 0; x < 4; ++x) {
                        const u32 first = tx + x * 2 < width ? tx + x * 2 : width - 1;
                        const u32 second = tx + x * 2 + 1 < width ? tx + x * 2 + 1 : width - 1;
                        const u8 high = u8((index(first, py) & 15u) << 4);
                        const u8 low = u8(index(second, py) & 15u);
                        out.push_back(u8(high | low));
                    }
                }
            }
        }
    }
    return out;
}

void push(std::vector<u8>& out, u8 r, u8 g, u8 b, u8 a) {
    out.push_back(r);
    out.push_back(g);
    out.push_back(b);
    out.push_back(a);
}

void expect_pixels(const gf::TplImage& image, const std::vector<u8>& expected, const char* what) {
    check(image.rgba.size() == expected.size(), what);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (image.rgba[i] != expected[i]) {
            std::fprintf(stderr, "tpl failure: %s (byte %zu: got %u, expected %u)\n", what, i,
                         unsigned(image.rgba[i]), unsigned(expected[i]));
            std::abort();
        }
    }
}

void expect_ok(const ImageSpec& spec, const std::vector<u8>& expected, const char* what) {
    const Built built = build({spec});
    gf::TplImage image;
    std::string error;
    check(gf::tpl_decode(view(built.bytes), 0, &image, &error), what);
    check(error.empty(), what);
    check(image.width == spec.width && image.height == spec.height, what);
    check(image.format == spec.format, what);
    check(image.image_count == 1 && image.image_index == 0, what);
    check(image.indexed == (spec.format == 8 || spec.format == 9), what);
    const u32 format_out = (spec.format == 8 || spec.format == 9) ? spec.palette_format : 0u;
    check(image.palette_format == format_out, what);
    expect_pixels(image, expected, what);
}

void expect_reject(const std::vector<u8>& bytes, u32 image_index, const char* what) {
    gf::TplImage image;
    image.width = 1234;
    image.height = 5678;
    image.rgba.assign(8, 0xAA);
    std::string error;
    const bool decoded = gf::tpl_decode(view(bytes), image_index, &image, &error);
    check(!decoded, what);
    check(!error.empty(), what);
    check(image.rgba.empty(), what);
    check(image.width == 0 && image.height == 0, what);
}

// The standard multi-tile RGB565 fixture, shared by the rejection cases.
Built rgb565_file() {
    const auto word = [](u32 x, u32 y) {
        return u16(((x * 2u + 1u) << 11) | ((y * 8u + 3u) << 5) | ((x + y) * 2u));
    };
    return build({make_spec(8, 4, 4, pack16(8, 4, word))});
}

std::array<u8, 4> rgba8_pixel(u32 p) {
    // R, G, B, A from the tile-local pixel index, all distinct per pixel.
    return {u8(0x80 + p * 5), u8(0x30 + p * 2), u8(0x40 + p * 7), u8(0x20 + p * 3)};
}

// ---- positive coverage ------------------------------------------------

int positive_cases() {
    int cases = 0;

    // RGB565, 8x4: two 4x4 tiles so tile addressing is exercised, not just
    // raster order.
    {
        const u32 width = 8, height = 4;
        const auto word = [](u32 x, u32 y) {
            return u16(((x * 2u + 1u) << 11) | ((y * 8u + 3u) << 5) | ((x + y) * 2u));
        };
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                push(expected, kRep5[x * 2u + 1u], kRep6[y * 8u + 3u], kRep5[(x + y) * 2u],
                     255);
            }
        }
        // Literal anchor for the last pixel: r5=15, g6=27, b5=20.
        check(expected[expected.size() - 4] == 123, "RGB565 anchor red");
        check(expected[expected.size() - 3] == 109, "RGB565 anchor green");
        check(expected[expected.size() - 2] == 165, "RGB565 anchor blue");
        expect_ok(make_spec(width, height, 4, pack16(width, height, word)), expected, "RGB565");
        ++cases;
    }

    // RGB5A3, 4x2: both the opaque (bit 15 set) and the RGBA4444-ish
    // three-bit-alpha branch.
    {
        const u16 words[8] = {0xFC00, 0x83E0, 0x7FFF, 0x7ABC, 0x0000, 0x0F00, 0x0123, 0x3ABC};
        const u32 width = 4, height = 2;
        const auto word = [&words](u32 x, u32 y) { return words[y * 4u + x]; };
        std::vector<u8> expected;
        push(expected, kRep5[31], kRep5[0], kRep5[0], 255);      // opaque red
        push(expected, kRep5[0], kRep5[31], kRep5[0], 255);      // opaque green
        push(expected, kRep4[15], kRep4[15], kRep4[15], kRep3[7]);  // opaque white
        push(expected, kRep4[10], kRep4[11], kRep4[12], kRep3[7]);
        push(expected, 0, 0, 0, 0);                              // alpha branch, zero
        push(expected, kRep4[15], 0, 0, kRep3[0]);
        push(expected, kRep4[1], kRep4[2], kRep4[3], kRep3[0]);
        push(expected, kRep4[10], kRep4[11], kRep4[12], kRep3[3]);
        expect_ok(make_spec(width, height, 5, pack16(width, height, word)), expected, "RGB5A3");
        ++cases;
    }

    // RGBA8, 4x4: AR plane then GB plane in each 64-byte tile.
    {
        const u32 width = 4, height = 4;
        std::vector<u8> pixels(64, 0);
        std::vector<u8> expected;
        for (u32 p = 0; p < 16; ++p) {
            const std::array<u8, 4> c = rgba8_pixel(p);
            pixels[p * 2] = c[3];
            pixels[p * 2 + 1] = c[0];
            pixels[32 + p * 2] = c[1];
            pixels[32 + p * 2 + 1] = c[2];
            push(expected, c[0], c[1], c[2], c[3]);
        }
        check(expected[0] == 128 && expected[1] == 48 && expected[2] == 64 && expected[3] == 32,
              "RGBA8 anchor pixel");
        expect_ok(make_spec(width, height, 6, pixels), expected, "RGBA8");
        ++cases;
    }

    // RGBA8, 5x3: partial 4x4 tiles must keep the 64-byte block layout.
    {
        const u32 width = 5, height = 3;
        std::vector<u8> pixels;
        std::vector<u8> expected(std::size_t(width) * height * 4, 0);
        for (u32 ty = 0; ty < height; ty += 4) {
            for (u32 tx = 0; tx < width; tx += 4) {
                for (u32 p = 0; p < 16; ++p) {
                    const std::array<u8, 4> c = rgba8_pixel(p);
                    pixels.push_back(c[3]);
                    pixels.push_back(c[0]);
                }
                for (u32 p = 0; p < 16; ++p) {
                    const std::array<u8, 4> c = rgba8_pixel(p);
                    pixels.push_back(c[1]);
                    pixels.push_back(c[2]);
                }
                for (u32 y = 0; y < 4; ++y) {
                    for (u32 x = 0; x < 4; ++x) {
                        const u32 px = tx + x, py = ty + y;
                        if (px >= width || py >= height) continue;
                        const std::array<u8, 4> c = rgba8_pixel(y * 4 + x);
                        const std::size_t at = (std::size_t(py) * width + px) * 4;
                        expected[at] = c[0];
                        expected[at + 1] = c[1];
                        expected[at + 2] = c[2];
                        expected[at + 3] = c[3];
                    }
                }
            }
        }
        expect_ok(make_spec(width, height, 6, pixels), expected, "RGBA8 5x3 partial tiles");
        ++cases;
    }

    // I4, 8x8: 4-bit intensity expands to all four channels.
    {
        const u32 width = 8, height = 8;
        const auto index = [](u32 x, u32 y) { return (x + 3u * y) & 15u; };
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                const u8 intensity = kRep4[index(x, y)];
                push(expected, intensity, intensity, intensity, intensity);
            }
        }
        expect_ok(make_spec(width, height, 0, pack8(width, height, 0, index)), expected, "I4");
        ++cases;
    }

    // I4, 5x3: an 8x8 block whose right and bottom edges are outside the image.
    {
        const u32 width = 5, height = 3;
        const auto index = [](u32 x, u32 y) { return (x * 3u + y) & 15u; };
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                const u8 intensity = kRep4[index(x, y)];
                push(expected, intensity, intensity, intensity, intensity);
            }
        }
        expect_ok(make_spec(width, height, 0, pack8(width, height, 0, index)), expected,
                  "I4 5x3 partial tile");
        ++cases;
    }

    // IA4, 8x4: intensity in the low nibble, alpha in the high nibble.
    {
        const u32 width = 8, height = 4;
        const auto index = [](u32 x, u32 y) {
            const u32 alpha = (x + 8u) & 15u;
            const u32 intensity = (y * 5u + x) & 15u;
            return (alpha << 4) | intensity;
        };
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                const u8 intensity = kRep4[(y * 5u + x) & 15u];
                const u8 alpha = kRep4[(x + 8u) & 15u];
                push(expected, intensity, intensity, intensity, alpha);
            }
        }
        expect_ok(make_spec(width, height, 2, pack8(width, height, 2, index)), expected, "IA4");
        ++cases;
    }

    // C4 with an RGB5A3 palette, 8x8: high nibble is the even-x pixel.
    {
        const u32 width = 8, height = 8;
        const u16 palette[4] = {0xFC00, 0x801F, 0x0F00, 0x7ABC};
        const u8 colors[4][4] = {{255, 0, 0, 255},
                                 {0, 0, 255, 255},
                                 {255, 0, 0, 0},
                                 {170, 187, 204, 255}};
        const auto index = [](u32 x, u32 y) { return (x + y) % 4u; };
        ImageSpec spec = make_spec(width, height, 8, pack8(width, height, 8, index));
        spec.palette.assign(palette, palette + 4);
        spec.palette_format = 2;
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                const u32 i = index(x, y);
                push(expected, colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
            }
        }
        expect_ok(spec, expected, "C4 with RGB5A3 palette");
        ++cases;
    }

    // C8 with an IA8 palette, 8x4: low byte intensity, high byte alpha.
    {
        const u32 width = 8, height = 4;
        const u16 palette[4] = {0x00FF, 0x40AA, 0xFF00, 0x80C3};
        const u8 colors[4][4] = {{255, 255, 255, 0},
                                 {170, 170, 170, 64},
                                 {0, 0, 0, 255},
                                 {195, 195, 195, 128}};
        const auto index = [](u32 x, u32 y) { return (x * 3u + y) % 4u; };
        ImageSpec spec = make_spec(width, height, 9, pack8(width, height, 9, index));
        spec.palette.assign(palette, palette + 4);
        spec.palette_format = 0;
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                const u32 i = index(x, y);
                push(expected, colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
            }
        }
        expect_ok(spec, expected, "C8 with IA8 palette");
        ++cases;
    }

    // C8 with an RGB565 palette, 8x4: the third palette format.
    {
        const u32 width = 8, height = 4;
        const auto index = [](u32 x, u32 y) { return (x + y) % 2u; };
        ImageSpec spec = make_spec(width, height, 9, pack8(width, height, 9, index));
        spec.palette = {0xF800, 0x07E0};
        spec.palette_format = 1;
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                if (index(x, y) == 0) {
                    push(expected, kRep5[31], kRep6[0], kRep5[0], 255);
                } else {
                    push(expected, kRep5[0], kRep6[63], kRep5[0], 255);
                }
            }
        }
        expect_ok(spec, expected, "C8 with RGB565 palette");
        ++cases;
    }

    // RGB565, 5x3: the explicitly requested non-multiple-of-tile-size case.
    {
        const u32 width = 5, height = 3;
        const auto word = [](u32 x, u32 y) {
            return u16(((x * 2u + 1u) << 11) | ((y * 8u + 3u) << 5) | ((x + y) * 2u));
        };
        std::vector<u8> expected;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                push(expected, kRep5[x * 2u + 1u], kRep6[y * 8u + 3u], kRep5[(x + y) * 2u],
                     255);
            }
        }
        expect_ok(make_spec(width, height, 4, pack16(width, height, word)), expected,
                  "RGB565 5x3 partial tiles");
        ++cases;
    }

    return cases;
}

// ---- rejection coverage -----------------------------------------------

int negative_cases() {
    int cases = 0;

    {
        Built file = rgb565_file();
        put32(file.bytes, 0, 0x0020AF31u);
        expect_reject(file.bytes, 0, "bad magic rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        file.bytes.resize(16);  // header plus four bytes of an eight-byte table entry
        expect_reject(file.bytes, 0, "truncated image table rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        file.bytes.resize(file.bytes.size() - 8);
        expect_reject(file.bytes, 0, "truncated pixel payload rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, file.image_header[0] + 8, u32(file.image_header[0]));
        expect_reject(file.bytes, 0, "overlapping image header and payload rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, file.image_header[0] + 8, 0);
        expect_reject(file.bytes, 0, "overlapping file header and payload rejected");
        ++cases;
    }
    {
        const auto index = [](u32, u32) { return 0u; };
        ImageSpec spec = make_spec(8, 8, 8, pack8(8, 8, 8, index));
        Built file = build({spec});
        put32(file.bytes, file.table + 4, 0);
        expect_reject(file.bytes, 0, "missing palette for format 8 rejected");
        ++cases;
    }
    {
        const auto word = [](u32, u32) { return u16(0xF800); };
        ImageSpec spec = make_spec(8, 4, 4, pack16(8, 4, word));
        spec.palette = {0xFC00};
        spec.palette_format = 2;
        expect_reject(build({spec}).bytes, 0, "unexpected palette for format 4 rejected");
        ++cases;
    }
    {
        const auto index = [](u32, u32) { return 3u; };
        ImageSpec spec = make_spec(8, 4, 9, pack8(8, 4, 9, index));
        spec.palette = {0x00FF, 0x40AA};
        spec.palette_format = 0;
        expect_reject(build({spec}).bytes, 0, "pixel index outside palette rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put16(file.bytes, file.image_header[0], 0);
        put16(file.bytes, file.image_header[0] + 2, 0);
        expect_reject(file.bytes, 0, "zero dimensions rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put16(file.bytes, file.image_header[0] + 2, 4097);
        expect_reject(file.bytes, 0, "oversized width rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put16(file.bytes, file.image_header[0], 4097);
        expect_reject(file.bytes, 0, "oversized height rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, file.image_header[0] + 4, 3);
        expect_reject(file.bytes, 0, "unsupported format 3 rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, file.image_header[0] + 4, 7);
        expect_reject(file.bytes, 0, "unsupported format 7 rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        expect_reject(file.bytes, 1, "image index out of range rejected");
        ++cases;
    }
    {
        const auto word = [](u32, u32) { return u16(0xF800); };
        ImageSpec first = make_spec(4, 4, 4, pack16(4, 4, word));
        const auto index = [](u32, u32) { return 0u; };
        ImageSpec second = make_spec(8, 8, 8, pack8(8, 8, 8, index));
        second.palette = {0xFC00};
        second.palette_format = 2;
        const Built file = build({first, second});
        expect_reject(file.bytes, 2, "multi-image out-of-range index rejected");
        ++cases;
    }
    {
        const auto index = [](u32, u32) { return 0u; };
        ImageSpec spec = make_spec(8, 4, 9, pack8(8, 4, 9, index));
        spec.palette = {0x00FF, 0x40AA};
        spec.palette_format = 0;
        Built file = build({spec});
        put32(file.bytes, file.palette_header[0] + 4, 3);
        expect_reject(file.bytes, 0, "unsupported palette format rejected");
        ++cases;
    }
    {
        const auto index = [](u32, u32) { return 0u; };
        ImageSpec spec = make_spec(8, 4, 9, pack8(8, 4, 9, index));
        spec.palette = {0x00FF, 0x40AA};
        spec.palette_format = 0;
        Built file = build({spec});
        put16(file.bytes, file.palette_header[0], 0);
        expect_reject(file.bytes, 0, "empty palette rejected");
        ++cases;
    }
    {
        const auto index = [](u32, u32) { return 0u; };
        ImageSpec spec = make_spec(8, 4, 9, pack8(8, 4, 9, index));
        spec.palette = {0x00FF, 0x40AA};
        spec.palette_format = 0;
        Built file = build({spec});
        put16(file.bytes, file.palette_header[0], 257);
        expect_reject(file.bytes, 0, "oversized palette rejected");
        ++cases;
    }
    {
        const auto index = [](u32, u32) { return 0u; };
        ImageSpec spec = make_spec(8, 4, 9, pack8(8, 4, 9, index));
        spec.palette = {0x00FF, 0x40AA};
        spec.palette_format = 0;
        Built file = build({spec});
        put32(file.bytes, file.palette_header[0] + 8, u32(file.bytes.size()));
        expect_reject(file.bytes, 0, "truncated palette data rejected");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, 4, 0);
        expect_reject(file.bytes, 0, "empty image table rejected");
        ++cases;
    }
    {
        // A null byte window must be refused, not dereferenced.
        gf::TplImage image;
        std::string error;
        check(!gf::tpl_decode(gf::ByteView{}, 0, &image, &error), "empty buffer rejected");
        check(!error.empty(), "empty buffer sets an error");
        ++cases;
    }

    return cases;
}

// ---- image table counting ---------------------------------------------

int count_cases() {
    int cases = 0;

    {
        const auto word = [](u32, u32) { return u16(0xF800); };
        const Built file = build({make_spec(4, 4, 4, pack16(4, 4, word))});
        u32 count = 99;
        std::string error;
        check(gf::tpl_image_count(view(file.bytes), &count, &error), "count single-image file");
        check(count == 1, "single-image count is 1");
        check(error.empty(), "valid count leaves no error");
        ++cases;
    }
    {
        const auto word = [](u32, u32) { return u16(0xF800); };
        const auto index = [](u32, u32) { return 0u; };
        std::vector<ImageSpec> images;
        images.push_back(make_spec(4, 4, 4, pack16(4, 4, word)));
        images.push_back(make_spec(8, 8, 8, pack8(8, 8, 8, index)));
        images.back().palette = {0xFC00};
        images.back().palette_format = 2;
        images.push_back(make_spec(8, 4, 0, pack8(8, 4, 0, index)));
        const Built file = build(images);
        u32 count = 0;
        std::string error;
        check(gf::tpl_image_count(view(file.bytes), &count, &error), "count multi-image file");
        check(count == 3, "multi-image count is 3");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, 0, 0u);
        u32 count = 0;
        std::string error;
        check(!gf::tpl_image_count(view(file.bytes), &count, &error),
              "count rejects bad magic");
        check(!error.empty(), "count failure sets an error");
        ++cases;
    }
    {
        Built file = rgb565_file();
        file.bytes.resize(16);
        u32 count = 0;
        std::string error;
        check(!gf::tpl_image_count(view(file.bytes), &count, &error),
              "count rejects a truncated table");
        check(!error.empty(), "truncated table count sets an error");
        ++cases;
    }
    {
        Built file = rgb565_file();
        put32(file.bytes, 4, 0u);
        u32 count = 0;
        std::string error;
        check(!gf::tpl_image_count(view(file.bytes), &count, &error),
              "count rejects an empty image table");
        check(!error.empty(), "empty table count sets an error");
        ++cases;
    }

    return cases;
}

// ---- multi-image decoding ---------------------------------------------

int multi_image_cases() {
    int cases = 0;

    const auto word = [](u32, u32) { return u16(0xF800); };
    const auto index = [](u32, u32) { return 0u; };
    ImageSpec first = make_spec(4, 4, 4, pack16(4, 4, word));
    ImageSpec second = make_spec(8, 8, 8, pack8(8, 8, 8, index));
    second.palette = {0xFC00};
    second.palette_format = 2;
    const Built file = build({first, second});

    gf::TplImage image;
    std::string error;
    check(gf::tpl_decode(view(file.bytes), 0, &image, &error), "decode first of two images");
    check(image.width == 4 && image.height == 4, "first image dimensions");
    check(image.format == 4 && image.image_index == 0 && image.image_count == 2,
          "first image metadata");
    check(image.rgba.size() == std::size_t(4) * 4 * 4, "first image byte count");
    ++cases;

    check(gf::tpl_decode(view(file.bytes), 1, &image, &error), "decode second of two images");
    check(image.width == 8 && image.height == 8, "second image dimensions");
    check(image.format == 8 && image.indexed && image.palette_format == 2,
          "second image metadata");
    check(image.image_index == 1 && image.image_count == 2, "second image index/count");
    check(image.rgba.size() == std::size_t(8) * 8 * 4, "second image byte count");
    ++cases;

    return cases;
}

// ---- real archive (read-only, skipped when absent) --------------------

struct RealResult {
    bool present = false;
    bool ok = true;
    unsigned files = 0;
    unsigned images = 0;
    unsigned long long bytes = 0;
};

RealResult real_archive() {
    RealResult result;
    std::ifstream file("original/extracted/files/afs_data.afs", std::ios::binary | std::ios::ate);
    if (!file) return result;
    result.present = true;

    const std::streampos end = file.tellg();
    if (end <= 0) {
        result.ok = false;
        return result;
    }
    file.seekg(0, std::ios::beg);
    std::vector<u8> data(static_cast<std::size_t>(end), u8(0));
    if (!file.read(reinterpret_cast<char*>(data.data()), std::streamsize(end))) {
        result.ok = false;
        return result;
    }

    gf::AfsArchive archive;
    std::string error;
    if (!archive.open(gf::ByteView{data.data(), data.size()}, &error)) {
        std::fprintf(stderr, "tpl real archive: %s\n", error.c_str());
        result.ok = false;
        return result;
    }

    for (std::size_t i = 0; i < archive.size(); ++i) {
        const gf::ByteView entry = archive.file(i);
        if (!entry.contains(0, 4) || entry.be32(0) != gf::kTplMagic) continue;
        ++result.files;

        u32 declared = 0;
        if (!gf::tpl_image_count(entry, &declared, &error)) {
            std::fprintf(stderr, "tpl real archive entry %zu: %s\n", i, error.c_str());
            result.ok = false;
            return result;
        }
        if (declared > 256) {
            std::fprintf(stderr, "tpl real archive entry %zu declares %u images\n", i,
                         unsigned(declared));
            result.ok = false;
            return result;
        }
        for (u32 image_index = 0; image_index < declared; ++image_index) {
            gf::TplImage image;
            if (!gf::tpl_decode(entry, image_index, &image, &error)) {
                std::fprintf(stderr, "tpl real archive entry %zu image %u: %s\n", i,
                             unsigned(image_index), error.c_str());
                result.ok = false;
                return result;
            }
            if (image.width < 1 || image.width > 4096 || image.height < 1 || image.height > 4096) {
                std::fprintf(stderr, "tpl real archive entry %zu image %u: dimensions\n", i,
                             unsigned(image_index));
                result.ok = false;
                return result;
            }
            if (image.rgba.size() != std::size_t(image.width) * image.height * 4) {
                std::fprintf(stderr, "tpl real archive entry %zu image %u: byte count\n", i,
                             unsigned(image_index));
                result.ok = false;
                return result;
            }
            ++result.images;
            result.bytes += image.rgba.size();
        }
    }
    return result;
}

} // namespace

int main() {
    const int positive = positive_cases();
    const int negative = negative_cases();
    const int counting = count_cases();
    const int multi = multi_image_cases();
    const RealResult real = real_archive();
    check(real.ok, "every real TPL image decodes");

    const int cases = positive + negative + counting + multi;
    std::printf("{\"tpl_cases\":%d,\"real_tpl_files\":%u,\"real_tpl_images\":%u,"
                "\"real_tpl_bytes\":%llu,\"all_passed\":true}\n",
                cases, real.files, real.images, real.bytes);
    return 0;
}
