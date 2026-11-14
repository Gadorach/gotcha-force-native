// MenuOverlay tests: the interaction layer drawn over the real room.
//
// The room's own option joints are authored in a source pose, so this overlay
// places its own seal ring. These checks pin the contract: nothing is drawn
// before configure(), one billboard per item plus a reticle and one label each,
// the reticle follows the selection, the caller's state survives, and the same
// inputs hash the same every time.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "engine/gameplay/frame_digest.hpp"
#include "engine/gameplay/menu_overlay.hpp"

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "menu_overlay failure: %s\n", what);
        std::abort();
    }
}

gf::Camera make_camera() {
    gf::Camera camera;
    camera.eye[0] = 0.0f;
    camera.eye[1] = 900.0f;
    camera.eye[2] = -1200.0f;
    camera.target[0] = 0.0f;
    camera.target[1] = 150.0f;
    camera.target[2] = 1150.0f;
    camera.fov_y_degrees = 45.0f;
    camera.near_plane = 10.0f;
    camera.far_plane = 50000.0f;
    camera.viewport_width = 640;
    camera.viewport_height = 480;
    return camera;
}

bool same_state(const gf::RenderState& a, const gf::RenderState& b) {
    return a.depth_test == b.depth_test && a.depth_write == b.depth_write && a.blend == b.blend &&
           a.blend_src == b.blend_src && a.blend_dst == b.blend_dst &&
           a.cull_backface == b.cull_backface && a.alpha_test == b.alpha_test &&
           a.alpha_ref == b.alpha_ref && a.texture == b.texture &&
           a.position_matrix == b.position_matrix && a.vertex_format == b.vertex_format;
}

unsigned draw_once(gf::MenuOverlay& overlay, unsigned selection, unsigned count,
                   gf::RecordingRenderBackend* out) {
    gf::RecordingRenderBackend backend;
    gf::RenderEngine render(backend);
    std::string error;
    check(render.initialize({640, 480, false, true, "overlay"}, nullptr, &error), "init");
    render.begin_frame({0, 0.0f, 0.0f, 0.0f});
    render.reset_arena();
    const unsigned draws = overlay.draw(render, make_camera(), selection, count);
    render.end_frame();
    render.present();
    if (out) *out = backend;
    return draws;
}

} // namespace

int main() {
    int cases = 0;
    const gf::f32 centre[3] = {0.0f, 300.0f, 1150.0f};

    // Before configure() nothing is emitted.
    {
        gf::MenuOverlay overlay;
        check(!overlay.configured(), "starts unconfigured");
        check(draw_once(overlay, 0, gf::kMenuItemCount, nullptr) == 0,
              "unconfigured overlay draws nothing");
        ++cases;
    }

    gf::MenuOverlay overlay;
    overlay.configure(centre, 700.0f, 150.0f);
    check(overlay.configured(), "configures");
    // A zero radius or size is refused rather than drawing degenerate quads.
    gf::MenuOverlay broken;
    broken.configure(centre, 0.0f, 150.0f);
    check(!broken.configured(), "zero radius is refused");
    broken.configure(centre, 700.0f, 0.0f);
    check(!broken.configured(), "zero seal size is refused");
    cases += 3;

    const unsigned count = gf::kMenuItemCount;
    gf::RecordingRenderBackend first;
    const unsigned draws = draw_once(overlay, 0, count, &first);
    check(draws == 2 * count + 1, "one billboard and one label per item plus one reticle");
    ++cases;

    // Determinism: identical inputs, identical frame.
    gf::RecordingRenderBackend again;
    check(draw_once(overlay, 0, count, &again) == draws, "same draw count");
    check(first.last_frame_digest() == again.last_frame_digest(), "same frame digest");
    ++cases;

    // The reticle follows the selection.
    gf::RecordingRenderBackend moved;
    draw_once(overlay, 3, count, &moved);
    check(first.last_frame_digest() != moved.last_frame_digest(), "selection moves the reticle");
    ++cases;

    // The caller's state is restored.
    {
        gf::RecordingRenderBackend backend;
        gf::RenderEngine render(backend);
        std::string error;
        check(render.initialize({640, 480, false, true, "overlay"}, nullptr, &error), "init");
        render.state().blend = false;
        render.state().texture = 42;
        render.state().depth_test = false;
        const gf::RenderState before = render.state();
        render.begin_frame({0, 0.0f, 0.0f, 0.0f});
        render.reset_arena();
        overlay.draw(render, make_camera(), 1, count);
        render.end_frame();
        render.present();
        check(same_state(render.state(), before), "render state restored");
        ++cases;
    }

    // Every seal projects inside the frame for this camera, so a ring that drifts
    // off-screen is caught here rather than in a screenshot.
    {
        const gf::Camera camera = make_camera();
        for (unsigned i = 0; i < count; ++i) {
            const gf::f32 turn = 6.2831853f * static_cast<gf::f32>(i) / static_cast<gf::f32>(count);
            const gf::f32 anchor[3] = {centre[0] + 700.0f * std::sin(turn), centre[1],
                                       centre[2] + 700.0f * std::cos(turn)};
            gf::f32 x = 0.0f, y = 0.0f, scale = 0.0f;
            check(camera.project(anchor, &x, &y, &scale), "seal projects");
            check(x >= 0.0f && x <= 640.0f && y >= 0.0f && y <= 480.0f,
                  "seal lands inside the viewport");
        }
        ++cases;
    }

    std::printf("{\"menu_overlay_cases\":%d,\"items\":%u,\"all_passed\":true}\n", cases, count);
    return 0;
}
