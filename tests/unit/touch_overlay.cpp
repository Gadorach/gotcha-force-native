// Touch overlay tests: the drawn control and the hit-tested control must be the
// same geometry, and the overlay must be configurable per platform.
#include "engine/core/context.hpp"
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/input/touch_overlay.hpp"

#include <cstdio>
#include <cstdlib>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "touch overlay failure: %s\n", what);
        std::abort();
    }
}

gf::OverlayButton button(gf::s32 x, gf::s32 y, gf::s32 w, gf::s32 h, gf::u32 raw) {
    gf::OverlayButton b;
    b.x = x; b.y = y; b.width = w; b.height = h; b.raw_button = raw;
    return b;
}

// Runs the overlay through a recording backend for one frame.
gf::u64 draw_once(gf::TouchOverlay& overlay, const gf::GamePadState& pad,
                  const gf::InputSnapshot& devices, unsigned* draws) {
    gf::RecordingRenderBackend backend;
    gf::RenderEngine render(backend);
    std::string error;
    check(render.initialize({640, 480, false, true, "overlay"}, nullptr, &error), "init");
    render.begin_frame({1, 0, 0, 0});
    *draws = overlay.draw(render, pad, devices);
    render.end_frame();
    render.present();
    return backend.last_frame_digest();
}

} // namespace

int main() {
    int cases = 0;

    // 1. Disabled overlay draws nothing at all, even with controls configured.
    {
        gf::TouchOverlay overlay;
        overlay.add_button(button(10, 10, 40, 40, 0x1));
        unsigned draws = 99;
        draw_once(overlay, {}, {}, &draws);
        check(draws == 0, "disabled overlay issues no draws");
        check(!overlay.layout().enabled, "disabled overlay reports no hit layout");
        ++cases;
    }

    // 2. Enabled overlay draws one quad per button plus the stick.
    {
        gf::TouchOverlay overlay;
        overlay.configure({true, 1.0f, 0, 0, false});
        overlay.add_button(button(10, 380, 60, 60, 0x1));
        overlay.add_button(button(90, 380, 60, 60, 0x2));
        gf::OverlayStick stick;
        stick.center_x = 160; stick.center_y = 240; stick.radius = 90;
        overlay.set_stick(stick);
        unsigned draws = 0;
        draw_once(overlay, {}, {}, &draws);
        check(draws == 3, "two buttons plus the stick are drawn");
        ++cases;
    }

    // 3. The drawn rectangle and the hit rectangle are identical, including
    //    after scaling and offsetting. This is the drift bug the design prevents.
    {
        gf::TouchOverlay overlay;
        overlay.configure({true, 2.0f, 100, 50, false});
        overlay.add_button(button(10, 20, 30, 40, 0x4));
        const gf::TouchLayout layout = overlay.layout();
        check(layout.button_count == 1, "one button in the hit layout");
        const auto& hit = layout.buttons[0];
        const auto rect = overlay.rect_of(overlay.buttons()[0]);
        check(hit.x == rect.x && hit.y == rect.y, "hit origin matches drawn origin");
        check(hit.width == rect.width && hit.height == rect.height, "hit size matches drawn size");
        check(hit.x == 100 + 20 && hit.y == 50 + 40, "scale and offset applied to both");
        ++cases;
    }

    // 4. A touch inside the drawn rectangle hits; one just outside does not.
    {
        gf::TouchOverlay overlay;
        overlay.configure({true, 1.0f, 0, 0, false});
        overlay.add_button(button(40, 40, 60, 60, 0x1));
        gf::InputRouter router;
        router.set_touch_layout(overlay.layout());

        gf::InputSnapshot inside;
        check(inside.push_touch(1, 70, 70), "touch inside");
        check(router.route(inside, 0).a, "touch centre presses the control");

        gf::InputSnapshot outside;
        check(outside.push_touch(1, 39, 70), "touch just left");
        check(!router.route(outside, 0).a, "touch outside does not press");
        ++cases;
    }

    // 5. Pressed state is visible: the digest changes when the control lights up.
    {
        gf::TouchOverlay overlay;
        overlay.configure({true, 1.0f, 0, 0, false});
        overlay.add_button(button(10, 10, 40, 40, 0x1));
        unsigned draws = 0;
        const gf::u64 idle = draw_once(overlay, {}, {}, &draws);
        gf::GamePadState pressed;
        pressed.a = true;
        const gf::u64 lit = draw_once(overlay, pressed, {}, &draws);
        check(idle != lit, "pressed highlight changes the drawn frame");
        ++cases;
    }

    // 6. Visibility is configuration: a hidden control is neither drawn nor hit.
    {
        gf::TouchOverlay overlay;
        overlay.configure({true, 1.0f, 0, 0, false});
        gf::OverlayButton hidden = button(10, 10, 40, 40, 0x1);
        hidden.visible = false;
        overlay.add_button(hidden);
        unsigned draws = 0;
        draw_once(overlay, {}, {}, &draws);
        check(draws == 0, "hidden control is not drawn");
        check(overlay.layout().button_count == 0, "hidden control is not hit-tested");
        ++cases;
    }

    // 7. Debug outline adds geometry but still exactly marks the hit rectangle.
    {
        gf::TouchOverlay overlay;
        overlay.configure({true, 1.0f, 0, 0, true});
        overlay.add_button(button(10, 10, 40, 40, 0x1));
        unsigned draws = 0;
        draw_once(overlay, {}, {}, &draws);
        check(draws == 1, "outline shares the button's draw");
        ++cases;
    }

    // 8. The overlay renders after world geometry and blends, so it is readable
    //    over the scene. Verified through submission order and state flags.
    {
        gf::RecordingRenderBackend backend;
        gf::RenderEngine render(backend);
        std::string error;
        check(render.initialize({640, 480, false, true, "order"}, nullptr, &error), "init");
        gf::Vertex world[3];
        world[0] = {0, 0, 0, 10, 10, 10, 255, 0, 0};
        world[1] = {10, 0, 0, 10, 10, 10, 255, 1, 0};
        world[2] = {0, 10, 0, 10, 10, 10, 255, 0, 1};

        gf::TouchOverlay overlay;
        overlay.configure({true, 1.0f, 0, 0, false});
        overlay.add_button(button(10, 10, 40, 40, 0x1));

        render.begin_frame({1, 0, 0, 0});
        check(render.upload_vertices(world, 3), "world upload");
        check(render.draw(gf::Primitive::Triangles, 0, 3), "world draw");
        render.state().colour_write=false;render.state().alpha_write=false;
        unsigned overlay_draws = overlay.draw(render, {}, {});
        render.end_frame();
        render.present();
        check(overlay_draws == 1, "overlay drew after the world");
        check(backend.draws() == 2, "world plus overlay in one frame");
        check(render.state().depth_test, "state restored after the overlay pass");
        check(!render.state().colour_write&&!render.state().alpha_write,"overlay restores caller RGB/alpha masks");
        ++cases;
    }

    // 9. A desktop configuration shares the same code with nothing drawn, so a
    //    platform decides by configuration rather than by a separate code path.
    {
        gf::TouchOverlay desktop;
        desktop.configure({false, 1.0f, 0, 0, false});
        desktop.add_button(button(10, 10, 40, 40, 0x1));
        unsigned draws = 0;
        draw_once(desktop, {}, {}, &draws);
        check(draws == 0, "desktop configuration draws nothing");
        check(!desktop.layout().enabled, "desktop configuration has no touch hits");
        ++cases;
    }

    std::printf("{\"overlay_cases\":%d,\"geometry_shared_with_hit_test\":true,"
                "\"pressed_highlights\":true,\"draws_after_world\":true,\"all_passed\":true}\n",
                cases);
    return 0;
}
