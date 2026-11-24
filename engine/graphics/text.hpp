#pragma once
// Bitmap text for the portable engine: the smallest readable UI text path that
// needs no asset pipeline.
//
// The font is an embedded 5x7 cell font: one constexpr table of 7 rows of 5
// bits per glyph, bit 0 = leftmost column. There is no font file, no texture and
// no atlas, so text works on a headless run and on any backend that can draw
// coloured triangles. Each lit pixel becomes one axis-aligned quad emitted as two
// triangles.
//
// Coordinates are in pixels with the origin at the top-left and y growing down,
// the same space as the pointer/touch coordinates the input seam reports.
//
// Design choices, stated so callers do not have to guess:
//   * One draw call per text block. Every glyph in a block shares the same
//     colour and no texture, so splitting per line or per glyph would only add
//     submissions. `draw()` returns 1 for any non-empty, non-blank block.
//   * The block is clipped deterministically when the vertex cap is reached:
//     whole glyphs are dropped, never half a glyph, and processing stops. The
//     same string always ends at the same glyph, so output stays deterministic.
//   * Vertices are built into a reusable member buffer (`draw()` is const; the
//     buffer is mutable scratch). Steady-state drawing reuses capacity and does
//     not allocate per call.
//   * UI state: blend on, depth test/write off, culling off, texture 0, identity
//     matrix in palette slot 0. The caller's blend factors and vertex format are
//     kept, mirroring TouchOverlay, so a UI pass configures them once.
//   * The previous RenderState is restored exactly after the draw, including the
//     matrix slot selector, so text cannot leak state into world geometry.
#include <cstddef>
#include <limits>
#include <string_view>
#include <vector>

#include "engine/core/types.hpp"
#include "engine/graphics/render_engine.hpp"

namespace gf {

// Renders ASCII UI text with an embedded bitmap font.
//
// Covered characters: space, '0'-'9', 'A'-'Z' and the punctuation
// `.,:!?/-'()%+`. Everything else (lowercase included) is drawn as a hollow
// "missing glyph" box rather than silently skipped.
class TextRenderer {
public:
    static constexpr s32 kGlyphColumns = 5;
    static constexpr s32 kGlyphRows = 7;
    static constexpr std::size_t kVerticesPerQuad = 6;

    // Hard ceiling on the vertices produced by one draw() call: 32,768 vertices
    // (5,461 quads). Text that would exceed it is clipped on a glyph boundary.
    // The densest glyph is 20 lit pixels (120 vertices), so a block always has
    // room for at least 273 whole glyphs.
    static constexpr std::size_t kMaxVertices = 32768;

    // Sizes are in font pixels: `scale` multiplies the whole cell (5x7) and both
    // spacings, so one Style scales consistently from a 640x480 window upwards.
    // `scale < 1` is treated as 1. Spacings may be negative for tight layout.
    struct Style {
        u8 r = 255;
        u8 g = 255;
        u8 b = 255;
        u8 a = 255;
        s32 scale = 2;
        s32 letter_spacing = 1;
        s32 line_spacing = 2;
    };

    // Distance from one line's top edge to the next line's top edge, in pixels.
    s32 line_height(const Style& style) const;

    // Width in pixels of the widest line of `text` as it would be drawn.
    // Multi-line text measures the widest line; an empty string measures 0.
    s32 measure_width(std::string_view text, const Style& style) const;

    // Appends quads for every lit glyph pixel and submits them. `x`,`y` are the
    // top-left corner of the first glyph cell. Returns the number of draws
    // issued: 1 for a block with any lit pixel, 0 for empty or all-blank text.
    unsigned draw(RenderEngine& render, std::string_view text, s32 x, s32 y,
                  const Style& style) const;

    // Draws with the block's measured width centred on `center_x`.
    unsigned draw_centered(RenderEngine& render, std::string_view text, s32 center_x,
                           s32 y, const Style& style) const;

    // True when `c` has a real glyph (' ' counts: it advances but draws nothing).
    static constexpr bool covers(char c) { return index_of(c) >= 0; }

private:
    friend class PortableTextRenderer;
    unsigned draw_impl(RenderEngine& render,std::string_view text,s32 x,s32 y,const Style& style,bool legacy_slot)const;

    struct Glyph {
        u8 rows[kGlyphRows];  // bit 0 = leftmost column, bit k = column k
    };

    static constexpr s32 kGlyphCount = 49;

    // Table order is fixed by index_of(): space, digits, uppercase, punctuation.
    static constexpr s32 index_of(char c) {
        if (c == ' ') return 0;
        if (c >= '0' && c <= '9') return 1 + (c - '0');
        if (c >= 'A' && c <= 'Z') return 11 + (c - 'A');
        switch (c) {
        case '.': return 37;
        case ',': return 38;
        case ':': return 39;
        case '!': return 40;
        case '?': return 41;
        case '/': return 42;
        case '-': return 43;
        case '\'': return 44;
        case '(': return 45;
        case ')': return 46;
        case '%': return 47;
        case '+': return 48;
        default: return -1;
        }
    }

    // One row per line; comments keep the source of each bit row reviewable.
    static constexpr Glyph kGlyphs[kGlyphCount] = {
        {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},  // ' '   ...../...../...../...../...../...../.....
        {{0x0E, 0x11, 0x19, 0x15, 0x13, 0x11, 0x0E}},  // '0'   .###./#...#/#..##/#.#.#/##..#/#...#/.###.
        {{0x04, 0x06, 0x04, 0x04, 0x04, 0x04, 0x0E}},  // '1'   ..#../.##../..#../..#../..#../..#../.###.
        {{0x0E, 0x11, 0x10, 0x08, 0x04, 0x02, 0x1F}},  // '2'   .###./#...#/....#/...#./..#../.#.../#####
        {{0x1F, 0x08, 0x04, 0x08, 0x10, 0x11, 0x0E}},  // '3'   #####/...#./..#../...#./....#/#...#/.###.
        {{0x08, 0x0C, 0x0A, 0x09, 0x1F, 0x08, 0x08}},  // '4'   ...#./..##./.#.#./#..#./#####/...#./...#.
        {{0x1F, 0x01, 0x0F, 0x10, 0x10, 0x11, 0x0E}},  // '5'   #####/#..../####./....#/....#/#...#/.###.
        {{0x0C, 0x02, 0x01, 0x0F, 0x11, 0x11, 0x0E}},  // '6'   ..##./.#.../#..../####./#...#/#...#/.###.
        {{0x1F, 0x10, 0x08, 0x04, 0x02, 0x02, 0x02}},  // '7'   #####/....#/...#./..#../.#.../.#.../.#...
        {{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},  // '8'   .###./#...#/#...#/.###./#...#/#...#/.###.
        {{0x0E, 0x11, 0x11, 0x1E, 0x10, 0x08, 0x06}},  // '9'   .###./#...#/#...#/.####/....#/...#./.##..
        {{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},  // 'A'   .###./#...#/#...#/#####/#...#/#...#/#...#
        {{0x0F, 0x11, 0x11, 0x0F, 0x11, 0x11, 0x0F}},  // 'B'   ####./#...#/#...#/####./#...#/#...#/####.
        {{0x0E, 0x11, 0x01, 0x01, 0x01, 0x11, 0x0E}},  // 'C'   .###./#...#/#..../#..../#..../#...#/.###.
        {{0x07, 0x09, 0x11, 0x11, 0x11, 0x09, 0x07}},  // 'D'   ###../#..#./#...#/#...#/#...#/#..#./###..
        {{0x1F, 0x01, 0x01, 0x0F, 0x01, 0x01, 0x1F}},  // 'E'   #####/#..../#..../####./#..../#..../#####
        {{0x1F, 0x01, 0x01, 0x0F, 0x01, 0x01, 0x01}},  // 'F'   #####/#..../#..../####./#..../#..../#....
        {{0x0E, 0x11, 0x01, 0x1D, 0x11, 0x11, 0x0E}},  // 'G'   .###./#...#/#..../#.###/#...#/#...#/.###.
        {{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},  // 'H'   #...#/#...#/#...#/#####/#...#/#...#/#...#
        {{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},  // 'I'   .###./..#../..#../..#../..#../..#../.###.
        {{0x1C, 0x08, 0x08, 0x08, 0x08, 0x09, 0x06}},  // 'J'   ..###/...#./...#./...#./...#./#..#./.##..
        {{0x11, 0x09, 0x05, 0x03, 0x05, 0x09, 0x11}},  // 'K'   #...#/#..#./#.#../##.../#.#../#..#./#...#
        {{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1F}},  // 'L'   #..../#..../#..../#..../#..../#..../#####
        {{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},  // 'M'   #...#/##.##/#.#.#/#.#.#/#...#/#...#/#...#
        {{0x11, 0x13, 0x15, 0x19, 0x11, 0x11, 0x11}},  // 'N'   #...#/##..#/#.#.#/#..##/#...#/#...#/#...#
        {{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},  // 'O'   .###./#...#/#...#/#...#/#...#/#...#/.###.
        {{0x0F, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x01}},  // 'P'   ####./#...#/#...#/####./#..../#..../#....
        {{0x0E, 0x11, 0x11, 0x11, 0x15, 0x09, 0x16}},  // 'Q'   .###./#...#/#...#/#...#/#.#.#/#..#./.##.#
        {{0x0F, 0x11, 0x11, 0x0F, 0x05, 0x09, 0x11}},  // 'R'   ####./#...#/#...#/####./#.#../#..#./#...#
        {{0x1E, 0x01, 0x01, 0x0E, 0x10, 0x10, 0x0F}},  // 'S'   .####/#..../#..../.###./....#/....#/####.
        {{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},  // 'T'   #####/..#../..#../..#../..#../..#../..#..
        {{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},  // 'U'   #...#/#...#/#...#/#...#/#...#/#...#/.###.
        {{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},  // 'V'   #...#/#...#/#...#/#...#/#...#/.#.#./..#..
        {{0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},  // 'W'   #...#/#...#/#...#/#.#.#/#.#.#/##.##/#...#
        {{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},  // 'X'   #...#/#...#/.#.#./..#../.#.#./#...#/#...#
        {{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},  // 'Y'   #...#/#...#/.#.#./..#../..#../..#../..#..
        {{0x1F, 0x10, 0x08, 0x04, 0x02, 0x01, 0x1F}},  // 'Z'   #####/....#/...#./..#../.#.../#..../#####
        {{0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}},  // '.'   ...../...../...../...../...../.##../.##..
        {{0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x02}},  // ','   ...../...../...../...../.##../.##../.#...
        {{0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00}},  // ':'   ...../.##../.##../...../.##../.##../.....
        {{0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},  // '!'   ..#../..#../..#../..#../..#../...../..#..
        {{0x0E, 0x11, 0x10, 0x08, 0x04, 0x00, 0x04}},  // '?'   .###./#...#/....#/...#./..#../...../..#..
        {{0x10, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01}},  // '/'   ....#/....#/...#./..#../.#.../#..../#....
        {{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},  // '-'   ...../...../...../#####/...../...../.....
        {{0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00}},  // '\''  ..#../..#../..#../...../...../...../.....
        {{0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},  // '('   ...#./..#../.#.../.#.../.#.../..#../...#.
        {{0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},  // ')'   .#.../..#../...#./...#./...#./..#../.#...
        {{0x13, 0x13, 0x08, 0x04, 0x02, 0x19, 0x19}},  // '%'   ##..#/##..#/...#./..#../.#.../#..##/#..##
        {{0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},  // '+'   ...../..#../..#../#####/..#../..#../.....
    };

    // Hollow box: visible, obviously not a normal glyph, never a silent skip.
    static constexpr Glyph kMissing{{0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F}};

    static constexpr const Glyph& glyph_for(char c) {
        const s32 index = index_of(c);
        return index >= 0 ? kGlyphs[index] : kMissing;
    }

    // Multiplies the whole cell, so 5x7, spacings and advances scale together.
    static constexpr s32 effective_scale(const Style& style) {
        return style.scale > 0 ? style.scale : 1;
    }

    // Unicode is a rendering detail for the caller: the engine font is ASCII, so
    // bytes >= 0x80 fall through to the missing box like any other unknown byte.
    static s64 advance_of(s32 scale, const Style& style) {
        return static_cast<s64>(kGlyphColumns + style.letter_spacing) * scale;
    }

    static s64 line_width(s64 glyphs, s32 scale, const Style& style) {
        if (glyphs <= 0) return 0;
        return (glyphs * kGlyphColumns + (glyphs - 1) * style.letter_spacing) * scale;
    }

    static s32 clamp_to_s32(s64 value) {
        if (value < 0) return 0;
        if (value > std::numeric_limits<s32>::max()) return std::numeric_limits<s32>::max();
        return static_cast<s32>(value);
    }

    void emit_quad(f32 x, f32 y, s32 size, const Style& style) const;

    // Reused between calls; draw() is const so the scratch buffer is mutable.
    mutable std::vector<Vertex> vertices_;
    static constexpr f32 kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
};

inline s32 TextRenderer::line_height(const Style& style) const {
    const s32 scale = effective_scale(style);
    const s64 height = static_cast<s64>(kGlyphRows + style.line_spacing) * scale;
    return clamp_to_s32(height > 0 ? height : 1);
}

inline s32 TextRenderer::measure_width(std::string_view text, const Style& style) const {
    const s32 scale = effective_scale(style);
    s64 widest = 0;
    s64 glyphs = 0;
    for (char c : text) {
        if (c == '\n') {
            const s64 width = line_width(glyphs, scale, style);
            if (width > widest) widest = width;
            glyphs = 0;
        } else {
            ++glyphs;
        }
    }
    const s64 width = line_width(glyphs, scale, style);
    if (width > widest) widest = width;
    return clamp_to_s32(widest);
}

inline void TextRenderer::emit_quad(f32 x, f32 y, s32 size, const Style& style) const {
    const f32 x0 = x;
    const f32 y0 = y;
    const f32 x1 = x + static_cast<f32>(size);
    const f32 y1 = y + static_cast<f32>(size);
    const f32 u = 0.0f;
    const f32 v = 0.0f;
    // Two triangles, winding-independent: culling is off while text draws.
    vertices_.push_back(Vertex{x0, y0, 0.0f, style.r, style.g, style.b, style.a, u, v});
    vertices_.push_back(Vertex{x1, y0, 0.0f, style.r, style.g, style.b, style.a, u, v});
    vertices_.push_back(Vertex{x1, y1, 0.0f, style.r, style.g, style.b, style.a, u, v});
    vertices_.push_back(Vertex{x0, y0, 0.0f, style.r, style.g, style.b, style.a, u, v});
    vertices_.push_back(Vertex{x1, y1, 0.0f, style.r, style.g, style.b, style.a, u, v});
    vertices_.push_back(Vertex{x0, y1, 0.0f, style.r, style.g, style.b, style.a, u, v});
}

inline unsigned TextRenderer::draw(RenderEngine& render, std::string_view text, s32 x, s32 y,
                                   const Style& style) const {
    return draw_impl(render,text,x,y,style,true);
}

inline unsigned TextRenderer::draw_impl(RenderEngine& render, std::string_view text, s32 x, s32 y,
                                        const Style& style,bool legacy_slot)const{
    if (text.empty()) return 0;

    const s32 scale = effective_scale(style);
    const s64 advance = advance_of(scale, style);
    const s32 line_advance = line_height(style);

    vertices_.clear();
    s64 pen_x = x;
    s64 pen_y = y;
    bool clipped = false;

    for (char c : text) {
        if (c == '\n') {
            pen_x = x;
            pen_y += line_advance;
            continue;
        }
        const Glyph& glyph = glyph_for(c);
        for (s32 row = 0; row < kGlyphRows && !clipped; ++row) {
            const u8 bits = glyph.rows[row];
            if (bits == 0) continue;
            for (s32 col = 0; col < kGlyphColumns; ++col) {
                if ((bits & static_cast<u8>(1u << col)) == 0) continue;
                if (vertices_.size() + kVerticesPerQuad > kMaxVertices) {
                    // Clip on a whole-glyph boundary: stop before emitting a
                    // partial glyph so the same text always ends at the same
                    // pixel.
                    clipped = true;
                    break;
                }
                const f32 px = static_cast<f32>(pen_x + static_cast<s64>(col) * scale);
                const f32 py = static_cast<f32>(pen_y + static_cast<s64>(row) * scale);
                emit_quad(px, py, scale, style);
            }
        }
        if (clipped) break;
        pen_x += advance;
    }

    // Blank text (only spaces/newlines) produces no geometry and no draw.
    if (vertices_.empty()) return 0;

    // Append into the frame arena so world geometry already queued this frame is
    // preserved; the returned base is where our block starts.
    const u32 base = render.append_vertices(vertices_.data(), vertices_.size());

    const RenderState saved = render.state();
    // UI pass: alpha blend on, depth off, no culling, untextured. Blend factors
    // and vertex format belong to the caller's UI pass (same contract as
    // TouchOverlay), so they are left untouched.
    render.state().blend = true;
    render.state().depth_test = false;
    render.state().depth_write = false;
    render.state().colour_write=true;render.state().alpha_write=true;
    render.state().cull_backface = false;
    render.state().texture = 0;
    if(legacy_slot)render.upload_matrix(0,kIdentity);
    render.state().position_matrix=legacy_slot?0:kIdentityPositionMatrix;

    const bool submitted =
        render.draw(Primitive::Triangles, base, static_cast<u32>(vertices_.size()));

    render.state() = saved;
    return submitted ? 1u : 0u;
}

inline unsigned TextRenderer::draw_centered(RenderEngine& render, std::string_view text,
                                            s32 center_x, s32 y, const Style& style) const {
    const s32 x = center_x - measure_width(text, style) / 2;
    return draw(render, text, x, y, style);
}

} // namespace gf
