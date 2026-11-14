// MenuScene tests: the layer that turns a decoded HSD room into draw calls.
//
// The decoder itself is covered by tests/unit/hsd.cpp; this file checks the
// scene-level contract: a rejected archive draws nothing, an accepted one
// uploads its textures, issues draws, restores the caller's render state and is
// deterministic for the same camera. The real room is used read-only when the
// private extraction is present.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "engine/gameplay/frame_digest.hpp"
#include "engine/gameplay/menu_scene.hpp"

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "menu_scene failure: %s\n", what);
        std::abort();
    }
}

struct Room {
    bool present = false;
    bool loaded = false;
    std::vector<gf::u8> bytes;
};

Room load_room() {
    Room room;
    std::ifstream file("local/analysis/arc/tl00_mdl.arc", std::ios::binary);
    if (!file) return room;
    room.present = true;
    room.bytes.assign(std::istreambuf_iterator<char>(file), {});
    return room;
}

// Counts vertex uploads: the scene appends every mesh's geometry and uploads it
// once per frame, instead of re-sending the whole arena for each of ~60 meshes.
struct CountingBackend final : gf::IRenderBackend {
    unsigned vertex_uploads = 0;
    bool initialize(const gf::RenderConfig&, gf::IWindowBackend*, std::string*) override {
        return true;
    }
    void shutdown() override {}
    gf::RenderCapabilities capabilities() const override { return {0, false, false, "counting"}; }
    bool upload_vertices(const gf::Vertex*, std::size_t) override {
        ++vertex_uploads;
        return true;
    }
    bool upload_texture(gf::u32, const gf::TextureDesc&) override { return true; }
    bool upload_matrix(gf::u32, const gf::f32*) override { return true; }
    void begin_frame(const gf::FrameDesc&) override {}
    void submit(const gf::DrawCall&) override {}
    void end_frame() override {}
    void present() override {}
};

bool same_state(const gf::RenderState& a, const gf::RenderState& b) {
    return a.depth_test == b.depth_test && a.depth_write == b.depth_write &&
           a.blend == b.blend && a.blend_src == b.blend_src && a.blend_dst == b.blend_dst &&
           a.cull_backface == b.cull_backface && a.alpha_test == b.alpha_test &&
           a.alpha_ref == b.alpha_ref && a.texture == b.texture &&
           a.position_matrix == b.position_matrix && a.vertex_format == b.vertex_format;
}

int negative_cases() {
    int cases = 0;
    gf::MenuScene scene;
    std::string error;
    check(!scene.load({}, &error), "an empty buffer is refused");
    check(!error.empty(), "the refusal explains itself");
    check(!scene.loaded(), "a refused scene is not loaded");
    ++cases;

    gf::RecordingRenderBackend backend;
    gf::RenderEngine render(backend);
    std::string init_error;
    check(render.initialize({640, 480, false, true, "menu_scene"}, nullptr, &init_error), "init");
    gf::Camera camera;
    camera.eye[2] = 10.0f;
    camera.viewport_width = 640;
    camera.viewport_height = 480;
    check(scene.draw(render, camera) == 0, "a refused scene draws nothing");
    ++cases;

    // Random bytes that are not an HSD archive.
    std::vector<gf::u8> junk(256, 0xAB);
    check(!scene.load({junk.data(), junk.size()}, &error), "junk is refused");
    ++cases;
    return cases;
}

int real_cases(const Room& room, unsigned* meshes, unsigned* textures) {
    int cases = 0;
    gf::MenuScene scene;
    std::string error;
    check(scene.load({room.bytes.data(), room.bytes.size()}, &error), "the real room loads");
    check(scene.loaded(), "the real room is loaded");
    check(scene.scene().meshes.size() > 60, "the room has meshes");
    check(scene.scene().textures.size() > 10, "the room has textures");
    ++cases;

    gf::f32 low[3], high[3];
    check(scene.bounds(low, high), "bounds are available");
    check(high[0] - low[0] > 1000.0f && high[2] - low[2] > 1000.0f,
          "the room is thousands of units across");
    ++cases;

    // The focus point is the dense part of the room (the desk), not its bounding
    // box centre — that is what the host uses to frame the menu.
    gf::f32 focus[3] = {0.0f, 0.0f, 0.0f};
    gf::f32 radius = 0.0f;
    check(scene.focus(focus, &radius), "the focus point is available");
    check(focus[2] > 0.0f && focus[2] < 3000.0f, "the focus sits inside the room");
    check(radius > 100.0f && radius < 4000.0f, "the focus radius is plausible");
    check(focus[1] > -1500.0f && focus[1] < 1500.0f, "the focus is near the desk height");
    ++cases;

    gf::RecordingRenderBackend backend;
    gf::RenderEngine render(backend);
    std::string init_error;
    check(render.initialize({640, 480, false, true, "menu_scene"}, nullptr, &init_error), "init");
    const unsigned uploaded = scene.upload_textures(render);
    check(uploaded == scene.scene().textures.size(), "every decoded texture is uploaded");
    ++cases;

    gf::Camera camera;
    camera.eye[0] = 0.0f;
    camera.eye[1] = 900.0f;
    camera.eye[2] = -1200.0f;
    camera.target[0] = 0.0f;
    camera.target[1] = 150.0f;
    camera.target[2] = 1150.0f;
    camera.fov_y_degrees = 45.0f;
    camera.near_plane = 10.0f;
    camera.far_plane = 200000.0f;
    camera.viewport_width = 640;
    camera.viewport_height = 480;

    // A caller's state must survive the scene draw.
    render.state().blend = true;
    render.state().blend_src = 5;
    render.state().blend_dst = 6;
    render.state().depth_test = false;
    render.state().texture = 99;
    const gf::RenderState before = render.state();

    render.begin_frame({0, 0.0f, 0.0f, 0.0f});
    render.reset_arena();
    const unsigned draws = scene.draw(render, camera);
    render.end_frame();
    render.present();
    check(draws > 0, "the room issues draws");
    check(same_state(render.state(), before), "the caller's render state is restored");
    ++cases;

    // Determinism: the same camera produces the same frame.
    gf::RecordingRenderBackend again_backend;
    gf::RenderEngine again(again_backend);
    check(again.initialize({640, 480, false, true, "menu_scene"}, nullptr, &init_error), "init2");
    gf::MenuScene second;
    check(second.load({room.bytes.data(), room.bytes.size()}, &error), "second load");
    second.upload_textures(again);
    again.begin_frame({0, 0.0f, 0.0f, 0.0f});
    again.reset_arena();
    second.draw(again, camera);
    again.end_frame();
    again.present();
    check(backend.last_frame_digest() == again_backend.last_frame_digest(),
          "two identical draws produce the same frame digest");
    ++cases;

    // One upload per frame, not one per mesh.
    {
        CountingBackend counting;
        gf::RenderEngine render(counting);
        check(render.initialize({640, 480, false, true, "menu_scene"}, nullptr, &init_error),
              "init counting");
        gf::MenuScene one;
        check(one.load({room.bytes.data(), room.bytes.size()}, &error), "load once");
        one.upload_textures(render);
        render.begin_frame({0, 0.0f, 0.0f, 0.0f});
        render.reset_arena();
        one.draw(render, camera);
        render.end_frame();
        render.present();
        check(counting.vertex_uploads == 1, "the scene uploads its geometry once per frame");
        ++cases;
    }

    // The archive's own option markers (untextured billboards) are hidden by
    // default: the file places them at two origins and the game moves them at
    // runtime, so drawing them there would be misleading. Asking for them must
    // draw more.
    {
        gf::RecordingRenderBackend hidden_backend;
        gf::RenderEngine hidden(hidden_backend);
        check(hidden.initialize({640, 480, false, true, "menu_scene"}, nullptr, &init_error),
              "init hidden");
        gf::MenuScene with_markers;
        check(with_markers.load({room.bytes.data(), room.bytes.size()}, &error), "load markers");
        with_markers.upload_textures(hidden);
        with_markers.set_show_source_markers(true);
        hidden.begin_frame({0, 0.0f, 0.0f, 0.0f});
        hidden.reset_arena();
        const unsigned with = with_markers.draw(hidden, camera);
        hidden.end_frame();
        hidden.present();
        check(with > draws, "showing the source markers draws more");
        ++cases;
    }

    // A camera move changes the frame (billboards and perspective both depend on it).
    gf::Camera moved = camera;
    moved.eye[0] = 400.0f;
    again.begin_frame({1, 0.0f, 0.0f, 0.0f});
    again.reset_arena();
    second.draw(again, moved);
    again.end_frame();
    again.present();
    check(backend.last_frame_digest() != again_backend.last_frame_digest(),
          "moving the camera changes the frame");
    ++cases;

    *meshes = static_cast<unsigned>(scene.scene().meshes.size());
    *textures = static_cast<unsigned>(scene.scene().textures.size());
    return cases;
}

} // namespace

int main() {
    const Room room = load_room();
    unsigned meshes = 0, textures = 0;
    int cases = negative_cases();
    if (room.present) cases += real_cases(room, &meshes, &textures);
    std::printf("{\"menu_scene_cases\":%d,\"real_present\":%s,\"real_meshes\":%u,"
                "\"real_textures\":%u,\"all_passed\":true}\n",
                cases, room.present ? "true" : "false", meshes, textures);
    return 0;
}
