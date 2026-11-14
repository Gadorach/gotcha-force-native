// Frame digest tests: a headless regression gate is only useful if the digest
// is deterministic and actually sensitive to the things we care about.
#include "engine/core/context.hpp"
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "engine/graphics/render_engine.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "frame digest failure: %s\n", what);
        std::abort();
    }
}

gf::Vertex tri(gf::f32 x) { return {x, 0.0f, 0.0f, 10, 20, 30, 255, 0.0f, 0.0f}; }

// Runs one frame through the real RenderEngine with the recording backend.
gf::u64 run_frame(gf::RecordingRenderBackend& backend, const std::vector<gf::Vertex>& vertices,
                  bool blend, gf::u32 texture) {
    gf::RenderEngine engine(backend);
    std::string error;
    check(engine.initialize({640, 480, false, true, "digest"}, nullptr, &error), "render init");
    engine.state().blend = blend;
    engine.state().texture = texture;
    // The digest window is one frame: uploads inside the window are counted,
    // which is the contract the recording backend documents.
    engine.begin_frame({7, 0.25f, 0.5f, 0.75f});
    check(engine.upload_vertices(vertices.data(), vertices.size()), "vertex upload");
    check(engine.draw(gf::Primitive::Triangles, 0, static_cast<gf::u32>(vertices.size())), "draw");
    engine.end_frame();
    engine.present();
    return backend.last_frame_digest();
}

} // namespace

int main() {
    std::vector<gf::Vertex> triangles{tri(0.0f), tri(1.0f), tri(2.0f)};

    // 1. Determinism: identical input, identical digest, repeatedly.
    gf::u64 first = 0;
    for (int i = 0; i < 3; ++i) {
        gf::RecordingRenderBackend backend;
        const gf::u64 digest = run_frame(backend, triangles, false, 0);
        if (i == 0) first = digest;
        else check(digest == first, "same frame hashes identically across runs");
    }
    check(first != 1469598103934665603ull, "digest is not the empty value");

    // 2. Sensitivity: each observable change must change the digest.
    struct Case { const char* what; std::vector<gf::Vertex> vertices; bool blend; gf::u32 texture; };
    const std::vector<Case> changes{
        {"vertex position", {tri(0.0f), tri(1.0f), tri(2.5f)}, false, 0},
        {"vertex colour", {{0.0f, 0.0f, 0.0f, 99, 20, 30, 255, 0.0f, 0.0f}, tri(1.0f), tri(2.0f)}, false, 0},
        {"vertex texcoord", {{0.0f, 0.0f, 0.0f, 10, 20, 30, 255, 0.5f, 0.0f}, tri(1.0f), tri(2.0f)}, false, 0},
        {"blend state", triangles, true, 0},
        {"texture binding", triangles, false, 9},
        {"vertex count", {tri(0.0f), tri(1.0f)}, false, 0},
    };
    for (const Case& c : changes) {
        gf::RecordingRenderBackend backend;
        const gf::u64 digest = run_frame(backend, c.vertices, c.blend, c.texture);
        check(digest != first, c.what);
    }
    for(unsigned channel=0;channel<2;++channel){
        auto changed=triangles;
        if(channel==0)changed[0].light[0]=128;else changed[0].specular[2]=50;
        gf::RecordingRenderBackend backend;
        check(run_frame(backend,changed,false,0)!=first,"lighting channels affect digest");
    }
    {
        gf::FrameDigest before,after;gf::DrawCall draw;
        before.add_draw(draw);draw.texture_after_lighting=true;after.add_draw(draw);
        check(before.value()!=after.value(),"texture/lighting order affects digest");
    }

    // 3. Order matters: the same geometry submitted in a different order differs.
    for(unsigned channel=0;channel<3;++channel){gf::FrameDigest before,after;gf::DrawCall draw;before.add_draw(draw);draw.post_rgb_add[channel]=1;after.add_draw(draw);
        check(before.value()!=after.value(),"each post-material RGB channel affects digest");}

    {
        gf::RecordingRenderBackend a;
        gf::RenderEngine engine(a);
        std::string error;
        check(engine.initialize({320, 240, false, true, "order"}, nullptr, &error), "init");
        check(engine.upload_vertices(triangles.data(), triangles.size()), "upload");
        engine.begin_frame({1, 0, 0, 0});
        engine.draw(gf::Primitive::Triangles, 0, 3);
        engine.draw(gf::Primitive::Lines, 0, 3);
        engine.end_frame();
        engine.present();
        const gf::u64 forward = a.last_frame_digest();

        gf::RecordingRenderBackend b;
        gf::RenderEngine engine2(b);
        check(engine2.initialize({320, 240, false, true, "order"}, nullptr, &error), "init2");
        check(engine2.upload_vertices(triangles.data(), triangles.size()), "upload2");
        engine2.begin_frame({1, 0, 0, 0});
        engine2.draw(gf::Primitive::Lines, 0, 3);
        engine2.draw(gf::Primitive::Triangles, 0, 3);
        engine2.end_frame();
        engine2.present();
        check(forward != b.last_frame_digest(), "submission order affects the digest");
    }

    // 4. Texture uploads are part of the digest.
    {
        gf::RecordingRenderBackend a;
        gf::RenderEngine engine(a);
        std::string error;
        check(engine.initialize({64, 64, false, true, "tex"}, nullptr, &error), "init");
        const gf::u8 pixels[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        engine.begin_frame({1, 0, 0, 0});
        check(engine.upload_texture(3, {2, 1, pixels}), "upload texture");
        engine.end_frame();
        engine.present();
        const gf::u64 with_texture = a.last_frame_digest();

        gf::RecordingRenderBackend b;
        gf::RenderEngine engine2(b);
        check(engine2.initialize({64, 64, false, true, "tex"}, nullptr, &error), "init2");
        engine2.begin_frame({1, 0, 0, 0});
        check(engine2.upload_texture(4, {2, 1, pixels}), "upload other texture");
        engine2.end_frame();
        engine2.present();
        check(with_texture != b.last_frame_digest(), "texture handle affects the digest");
    }

    // 5. Alpha test is observable: the flag and the reference value both matter.
    {
        auto digest_with_alpha = [&](bool test_on, gf::u8 reference) {
            gf::RecordingRenderBackend backend;
            gf::RenderEngine engine(backend);
            std::string error;
            check(engine.initialize({64, 64, false, true, "alpha"}, nullptr, &error), "init alpha");
            check(engine.upload_vertices(triangles.data(), triangles.size()), "upload alpha");
            engine.begin_frame({1, 0, 0, 0});
            engine.state().alpha_test = test_on;
            engine.state().alpha_ref = reference;
            check(engine.draw(gf::Primitive::Triangles, 0, 3), "alpha draw");
            engine.end_frame();
            engine.present();
            return backend.last_frame_digest();
        };
        const gf::u64 off = digest_with_alpha(false, 0);
        const gf::u64 on = digest_with_alpha(true, 0);
        const gf::u64 on_high = digest_with_alpha(true, 128);
        check(off != on, "alpha test flag affects the digest");
        check(on != on_high, "alpha reference value affects the digest");
        check(off != digest_with_alpha(false, 128), "alpha ref is hashed even when the test is off");
    }

    // Write masks are part of state capture/digests, even for depth-only work.
    {
        gf::FrameDigest a,b,c;gf::DrawCall call;a.add_draw(call);
        call.colour_write=false;b.add_draw(call);call.alpha_write=false;c.add_draw(call);
        check(a.value()!=b.value()&&b.value()!=c.value(),"RGB and alpha write masks affect digest independently");
    }
    // 6. The digest is usable through the real Context with a swapped backend.
    {
        gf::headless::Bundle headless;
        gf::RecordingRenderBackend recorder;
        gf::Backends backends = headless.get();
        backends.render = &recorder;
        check(backends.complete(), "swapped set complete");
        gf::Context context(backends, 60);
        gf::RenderEngine engine(recorder);
        std::string error;
        check(engine.initialize({640, 480, false, true, "ctx"}, context.backends().window, &error), "init");
        check(engine.upload_vertices(triangles.data(), triangles.size()), "upload");
        context.add_elapsed(context.tick_ns() * 2);
        int ticks = 0;
        while (context.tick_ready()) { context.consume_tick(); ++ticks; }
        check(ticks == 2, "two ticks advanced");
        engine.begin_frame({0, 0, 0, 0});
        engine.draw(gf::Primitive::Triangles, 0, 3);
        engine.end_frame();
        engine.present();
        check(recorder.draws() == 1, "recorder saw exactly one draw");
        check(recorder.last_frame_digest() != 0, "digest produced through Context");
    }

    std::printf("{\"determinism\":true,\"sensitivity_cases\":%zu,\"order_sensitive\":true,"
                "\"texture_included\":true,\"context_integration\":true,\"all_passed\":true}\n",
                changes.size());
    return 0;
}
