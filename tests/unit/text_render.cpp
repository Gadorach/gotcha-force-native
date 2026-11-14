// Bitmap text tests: the font table, the measure/draw geometry contract, UI
// state hygiene and determinism. Runs headless through the real RenderEngine
// with a recording backend; no GPU, window or game data involved.
#include "engine/graphics/text.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "text render failure: %s\n", what);
        std::abort();
    }
}

// Minimal recording backend: counts vertices and keeps the submitted calls so a
// test can inspect the state a draw was issued with. Implements every pure
// virtual of IRenderBackend.
class RecordingBackend final : public gf::IRenderBackend {
public:
    std::vector<gf::Vertex> vertices;
    std::vector<gf::DrawCall> calls;
    int frames = 0;
    std::size_t uploads = 0;

    bool initialize(const gf::RenderConfig&, gf::IWindowBackend*, std::string*) override {
        return true;
    }
    void shutdown() override {}
    gf::RenderCapabilities capabilities() const override { return {4096, false, false, "recording"}; }
    bool upload_vertices(const gf::Vertex* v, std::size_t count) override {
        if (!v && count) return false;
        vertices.assign(v, v + count);
        ++uploads;
        return true;
    }
    bool upload_texture(gf::u32 handle, const gf::TextureDesc& desc) override {
        return handle != 0 && desc.width != 0 && desc.height != 0 && desc.pixels != nullptr;
    }
    bool upload_matrix(gf::u32, const gf::f32* matrix) override { return matrix != nullptr; }
    void begin_frame(const gf::FrameDesc&) override { ++frames; }
    void submit(const gf::DrawCall& call) override { calls.push_back(call); }
    void end_frame() override {}
    void present() override {}
};

struct Capture {
    unsigned draws = 0;
    std::size_t vertices = 0;
    std::vector<gf::Vertex> verts;
    std::vector<gf::DrawCall> calls;
    gf::u64 digest = 0;
    bool has_bounds = false;
    gf::f32 min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    gf::RenderState before{};
    gf::RenderState after{};
};

bool same_state(const gf::RenderState& a, const gf::RenderState& b) {
    return a.depth_test == b.depth_test && a.depth_write == b.depth_write &&
           a.blend == b.blend && a.blend_src == b.blend_src && a.blend_dst == b.blend_dst &&
           a.cull_backface == b.cull_backface && a.texture == b.texture &&
           a.position_matrix == b.position_matrix && a.vertex_format == b.vertex_format;
}

// FNV-1a over every vertex field: a cheap digest-like total for determinism.
gf::u64 vertex_digest(const std::vector<gf::Vertex>& verts) {
    gf::u64 hash = 1469598103934665603ull;
    auto mix = [&hash](const void* data, std::size_t size) {
        const auto* bytes = static_cast<const gf::u8*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= 1099511628211ull;
        }
    };
    for (const gf::Vertex& v : verts) {
        mix(&v.x, sizeof v.x);
        mix(&v.y, sizeof v.y);
        mix(&v.z, sizeof v.z);
        mix(&v.r, sizeof v.r);
        mix(&v.g, sizeof v.g);
        mix(&v.b, sizeof v.b);
        mix(&v.a, sizeof v.a);
        mix(&v.u, sizeof v.u);
        mix(&v.v, sizeof v.v);
    }
    return hash;
}

// Draws one block through a fresh engine whose state starts from a non-UI
// configuration, so both "UI state applied" and "state restored" are observable.
Capture run(const gf::TextRenderer& text, std::string_view view, gf::s32 x, gf::s32 y,
            const gf::TextRenderer::Style& style, bool centered = false) {
    RecordingBackend backend;
    gf::RenderEngine engine(backend);
    std::string error;
    check(engine.initialize({640, 480, false, true, "text"}, nullptr, &error), "render init");

    gf::RenderState prior{};
    prior.depth_test = true;
    prior.depth_write = true;
    prior.colour_write=false;prior.alpha_write=false;
    prior.blend = false;
    prior.blend_src = 3;
    prior.blend_dst = 4;
    prior.cull_backface = true;
    prior.texture = 7;
    prior.position_matrix = 5;
    prior.vertex_format = 11;
    engine.state() = prior;

    engine.begin_frame({1, 0.0f, 0.0f, 0.0f});
    Capture cap;
    cap.before = prior;
    cap.draws = centered ? text.draw_centered(engine, view, x, y, style)
                         : text.draw(engine, view, x, y, style);
    cap.after = engine.state();
    engine.end_frame();
    engine.present();

    cap.calls = backend.calls;
    cap.verts = backend.vertices;
    cap.vertices = cap.verts.size();
    cap.digest = vertex_digest(cap.verts);
    if (!cap.verts.empty()) {
        cap.has_bounds = true;
        cap.min_x = cap.max_x = cap.verts[0].x;
        cap.min_y = cap.max_y = cap.verts[0].y;
        for (const gf::Vertex& v : cap.verts) {
            if (v.x < cap.min_x) cap.min_x = v.x;
            if (v.x > cap.max_x) cap.max_x = v.x;
            if (v.y < cap.min_y) cap.min_y = v.y;
            if (v.y > cap.max_y) cap.max_y = v.y;
        }
    }
    return cap;
}

} // namespace

int main() {
    gf::TextRenderer text;
    gf::TextRenderer::Style style;  // defaults: scale 2, spacing 1/2
    int cases = 0;

    // 1. Empty text is a no-op: no arena growth, no draw, no call.
    {
        Capture cap = run(text, "", 10, 10, style);
        check(cap.draws == 0, "empty text issues no draw");
        check(cap.vertices == 0, "empty text emits no vertices");
        check(cap.calls.empty(), "empty text submits no call");
        check(cap.after.texture == 7 && cap.after.position_matrix == 5,
              "empty text leaves earlier state alone");
        ++cases;
    }

    // 2. One glyph: whole quads only, one draw for the block.
    unsigned draws_per_string = 0;
    {
        Capture cap = run(text, "A", 10, 10, style);
        draws_per_string = cap.draws;
        check(cap.draws == 1, "single glyph block issues exactly one draw");
        check(cap.calls.size() == 1, "single glyph block submits one call");
        check(cap.vertices == 18 * gf::TextRenderer::kVerticesPerQuad,
              "'A' emits one quad per lit pixel (18 pixels)");
        check(cap.vertices % gf::TextRenderer::kVerticesPerQuad == 0,
              "vertices come in whole quads");
        check(cap.calls[0].primitive == gf::Primitive::Triangles, "glyph pixels are triangles");
        check(cap.calls[0].colour_write&&cap.calls[0].alpha_write&&!cap.after.colour_write&&!cap.after.alpha_write,"text enables UI writes then restores inherited mask state");
        check(cap.calls[0].vertex_first == 0 && cap.calls[0].vertex_count == cap.vertices,
              "one draw covers the whole block");
        ++cases;
    }

    // 3. Measurement grows with length, with scale, and matches drawn extents.
    {
        const gf::s32 one = text.measure_width("A", style);
        const gf::s32 two = text.measure_width("AA", style);
        const gf::s32 three = text.measure_width("AAA", style);
        check(one > 0 && two > one && three > two, "width grows with string length");
        check(text.measure_width("", style) == 0, "empty string measures zero");

        gf::TextRenderer::Style large = style;
        large.scale = 4;
        const gf::s32 scaled = text.measure_width("AAA", large);
        check(scaled == three * 2, "width scales with Style::scale");
        check(text.line_height(large) == text.line_height(style) * 2, "line height scales too");

        Capture small = run(text, "HH", 0, 0, style);
        gf::TextRenderer::Style wide = style;
        wide.scale = style.scale * 2;
        Capture big = run(text, "HH", 0, 0, wide);
        check(text.measure_width("HH", style) ==
                  static_cast<gf::s32>(small.max_x - small.min_x),
              "measured width equals drawn width for a full-cell glyph");
        check(static_cast<gf::s32>(big.max_x - big.min_x) ==
                  static_cast<gf::s32>(small.max_x - small.min_x) * 2,
              "doubling scale draws proportionally wider text");
        ++cases;
    }

    // 4. Newlines advance by line_height and keep drawing per line.
    {
        const gf::s32 line = text.line_height(style);
        check(line == (gf::TextRenderer::kGlyphRows + style.line_spacing) * style.scale,
              "line height follows the glyph cell plus spacing");
        Capture one = run(text, "A", 10, 10, style);
        Capture two = run(text, "A\nA", 10, 10, style);
        check(two.draws == 1, "multi-line block is still one draw");
        check(two.vertices == one.vertices * 2, "two lines draw both glyphs");
        check(two.max_y - two.min_y ==
                  static_cast<gf::f32>(line + gf::TextRenderer::kGlyphRows * style.scale),
              "two lines span one line advance plus one glyph cell");

        const gf::f32 split = static_cast<gf::f32>(10 + line);
        std::size_t first_band = 0;
        std::size_t second_band = 0;
        for (const gf::Vertex& v : two.verts) {
            if (v.y < split) ++first_band;
            else ++second_band;
        }
        check(first_band == one.vertices && second_band == one.vertices,
              "each line owns exactly its own glyph pixels");
        ++cases;
    }

    // 5. Unknown characters draw a visible missing box, not nothing.
    {
        Capture space = run(text, " ", 0, 0, style);
        Capture missing = run(text, "@", 0, 0, style);
        check(space.draws == 0 && space.vertices == 0, "a space is blank");
        check(missing.draws == 1 && missing.vertices > 0, "unknown character is drawn");
        check(missing.vertices != space.vertices, "missing glyph differs from a space");
        check(missing.vertices == 20 * gf::TextRenderer::kVerticesPerQuad,
              "missing glyph is the 20-pixel hollow box");
        check(gf::TextRenderer::covers(' ') && gf::TextRenderer::covers('A') &&
                  gf::TextRenderer::covers('%'),
              "covered characters are reported");
        check(!gf::TextRenderer::covers('@'), "unknown characters are not covered");
        check(!gf::TextRenderer::covers('a'), "lowercase falls back to the missing glyph");

        // Every covered character must produce geometry (space excepted).
        const std::string covered =
            " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:!?/-'()%+";
        for (char c : covered) {
            if (c == ' ') {
                check(gf::TextRenderer::covers(c), "space is a real glyph");
                check(run(text, std::string_view(&c, 1), 0, 0, style).vertices == 0,
                      "space draws no pixels");
                continue;
            }
            check(gf::TextRenderer::covers(c), "font covers the advertised alphabet");
            check(run(text, std::string_view(&c, 1), 0, 0, style).vertices > 0,
                  "covered glyph is visible");
        }
        ++cases;
    }

    // 6. Centering is symmetric about center_x and equals draw() at the
    //    measured offset.
    {
        const gf::s32 center = 320;
        Capture centered = run(text, "HH", center, 10, style, true);
        check(centered.draws == 1, "centered text still draws");
        const gf::f32 middle = (centered.min_x + centered.max_x) / 2.0f;
        check(middle > static_cast<gf::f32>(center) - 1.0f &&
                  middle < static_cast<gf::f32>(center) + 1.0f,
              "centered text is symmetric about center_x");

        const gf::s32 offset = center - text.measure_width("HH", style) / 2;
        Capture manual = run(text, "HH", offset, 10, style);
        check(centered.digest == manual.digest && centered.vertices == manual.vertices,
              "draw_centered matches draw at the measured x");
        ++cases;
    }

    // 7. UI state is applied for the draw and fully restored afterwards.
    {
        Capture cap = run(text, "HI", 5, 5, style);
        check(cap.calls.size() == 1, "block submits exactly one call");
        const gf::DrawCall& call = cap.calls[0];
        check(call.blend, "blending is on for text");
        check(!call.depth_test, "depth test is off for text");
        check(!call.depth_write, "depth write is off for text");
        check(!call.cull_backface, "culling is off for text");
        check(call.texture == 0, "text is untextured");
        check(call.position_matrix == 0, "text draws with the identity matrix slot");
        check(call.blend_src == 3 && call.blend_dst == 4,
              "caller blend factors are preserved, not overwritten");
        check(same_state(cap.after, cap.before), "render state is restored after drawing");
        ++cases;
    }

    // 8. Determinism: same text, same vertices, regardless of buffer reuse.
    {
        Capture a = run(text, "SCORE: 1200!", 3, 7, style);
        Capture b = run(text, "SCORE: 1200!", 3, 7, style);
        check(a.vertices > 0 && a.draws == b.draws && a.vertices == b.vertices,
              "identical input produces identical vertex counts");
        check(a.digest == b.digest, "identical input produces identical vertex data");

        Capture noise = run(text, "ZZZ", 0, 0, style);
        check(noise.vertices > 0, "intermediate draw happened");
        Capture c = run(text, "SCORE: 1200!", 3, 7, style);
        check(c.digest == a.digest && c.vertices == a.vertices,
              "reusing the scratch buffer does not change output");
        ++cases;
    }

    // 9. Vertex cap: long text is clipped on a glyph boundary, deterministically.
    {
        const std::string huge(1000, 'M');
        Capture a = run(text, huge, 0, 0, style);
        Capture b = run(text, huge, 0, 0, style);
        check(a.vertices <= gf::TextRenderer::kMaxVertices, "vertex cap is respected");
        check(a.vertices > 0, "clipped text still draws");
        check(gf::TextRenderer::kMaxVertices - a.vertices < gf::TextRenderer::kVerticesPerQuad,
              "clipping stops at the last whole quad of the cap");
        check(a.vertices % gf::TextRenderer::kVerticesPerQuad == 0, "clipping keeps whole quads");
        check(a.vertices == b.vertices && a.digest == b.digest,
              "clipping is deterministic for the same input");

        Capture modest = run(text, "HELLO WORLD", 0, 0, style);
        check(modest.vertices > 0 && modest.vertices < gf::TextRenderer::kMaxVertices,
              "ordinary UI text is not clipped");
        gf::s32 expected = 0;
        for (char c : std::string_view("HELLO WORLD")) {
            if (c == ' ') continue;
            expected += static_cast<gf::s32>(run(text, std::string_view(&c, 1), 0, 0, style).vertices);
        }
        check(modest.vertices == static_cast<std::size_t>(expected),
              "block vertex count is the sum of its glyphs");
        ++cases;
    }

    std::printf("{\"text_cases\":%d,\"draws_per_string\":%u,\"all_passed\":true}\n", cases,
                draws_per_string);
    return 0;
}
