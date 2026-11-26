// Input router tests: one snapshot from any mix of devices must reach the game
// identically, and the touch overlay must never fight a physical controller.
#include "engine/core/headless_backends.hpp"
#include "engine/input/input_router.hpp"
#include "platform/pc/keyboard_layout.hpp"

#include <cstdio>
#include <cstdlib>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "input router failure: %s\n", what);
        std::abort();
    }
}

gf::TouchLayout phone_layout() {
    gf::TouchLayout layout;
    layout.enabled = true;
    // D-pad style buttons in a 640x480 reference space.
    layout.add_button({40, 380, 60, 60, 0x1});        // A
    layout.add_button({110, 380, 60, 60, 0x2});       // B
    layout.has_stick = true;
    layout.stick = {160, 240, 100, 0, 1};
    return layout;
}

} // namespace

int main() {
    gf::InputRouter router;
    int cases = 0;

    {
        gf::InputSnapshot devices;devices.port_count=4;
        devices.pad(0).buttons=1;devices.pad(1).buttons=2;devices.pad(2).buttons=4;devices.pad(3).buttons=8;
        for(unsigned port=0;port<4;++port)check(router.port(port).device_pad==port,"default ports map one-to-one");
        check(router.route(devices,1).b&&!router.route(devices,1).a,"default second port is independent");
        check(router.route(devices,2).x&&!router.route(devices,2).a,"default third port is independent");
        check(router.route(devices,3).y&&!router.route(devices,3).a,"default fourth port is independent");
        gf::InputRouter remap;gf::PortBinding shared;shared.device_pad=0;remap.set_port(3,shared);
        check(remap.route(devices,3).a,"explicit shared/remapped bindings remain supported");++cases;
    }

    // 1. Keyboard-only (device pad 0) reaches the game with no touch layout.
    {
        gf::InputSnapshot devices;
        devices.port_count = 1;
        devices.pad(0).buttons = 0x1;              // A
        const gf::GamePadState pad = router.route(devices, 0);
        check(pad.a, "keyboard A reaches the game");
        check(!pad.b, "unpressed button stays up");
        ++cases;
    }

    // 1b. The second keyboard pad reaches port 1 and never leaks to port 0.
    {
        gf::InputSnapshot devices;
        devices.port_count = 2;
        gf::pc::KeyboardPad second;
        second.a = true;
        second.axis_down = true;
        gf::pc::map_keyboard_pad(second, &devices.pad(1));
        const gf::GamePadState first = router.route(devices, 0);
        const gf::GamePadState other = router.route(devices, 1);
        check(!first.a && first.stick_y == 0, "P2 keyboard pad does not leak to port 0");
        check(other.a && other.stick_y > 0, "P2 keyboard pad reaches port 1");
        ++cases;
    }

    // 2. Controller on a second port is independent of port 0.
    {
        gf::InputRouter multi;
        gf::PortBinding second;
        second.device_pad = 1;
        multi.set_port(1, second);
        gf::InputSnapshot devices;
        devices.port_count = 2;
        devices.pad(0).buttons = 0x1;              // port 0: A
        devices.pad(1).buttons = 0x2;              // port 1: B
        check(multi.route(devices, 0).a && !multi.route(devices, 0).b, "port 0 sees only its pad");
        check(multi.route(devices, 1).b && !multi.route(devices, 1).a, "port 1 sees only its pad");
        ++cases;
    }

    // 3. Touch overlay drives buttons and the stick when no pad is active.
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot devices;
        devices.port_count = 1;
        check(devices.push_touch(1, 70, 410), "touch inside A");
        const gf::GamePadState pad = touch.route(devices, 0);
        check(pad.a, "touch in the A rectangle presses A");
        check(!pad.b, "touch outside B does not press B");
        ++cases;
    }

    // 4. Touch stick produces analogue input, scaled by the normal map.
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot devices;
        devices.push_touch(1, 210, 240);           // right of centre by half radius
        const gf::GamePadState pad = touch.route(devices, 0);
        check(pad.stick_x > 40 && pad.stick_x < 90, "half deflection maps to roughly half scale");
        check(pad.stick_y == 0, "centred vertically");
        ++cases;
    }

    // 5. Touch is ignored while a physical pad is active, and deadzone applies
    //    to both paths identically.
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot devices;
        devices.pad(0).buttons = 0x4;              // physical X pressed
        check(devices.push_touch(1, 70, 410), "touch inside A while pad active");
        const gf::GamePadState pad = touch.route(devices, 0);
        check(pad.x, "physical button wins");
        check(!pad.a, "overlay does not add buttons while a pad is active");
        ++cases;
    }
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot devices;
        devices.push_touch(1, 160, 240);           // exactly centre: inside deadzone
        const gf::GamePadState pad = touch.route(devices, 0);
        check(pad.stick_x == 0 && pad.stick_y == 0, "deadzone produces exact zero");
        ++cases;
    }

    // 6. Small stick noise from a pad does not suppress the overlay, but a real
    //    deflection does.
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot quiet;
        quiet.pad(0).axis[0] = 1000;               // noise
        check(quiet.push_touch(1, 70, 410), "touch with noisy pad");
        check(touch.route(quiet, 0).a, "noise does not disable the overlay");

        gf::InputSnapshot real;
        real.pad(0).axis[0] = 20000;               // deliberate
        check(real.push_touch(1, 70, 410), "touch with real deflection");
        check(!touch.route(real, 0).a, "real deflection disables the overlay");
        ++cases;
    }

    // 7. Negative analogue deflection survives the merge (a naive max would
    //    discard it).
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot devices;
        devices.pad(0).axis[0] = -20000;           // pad: hard left
        devices.pad(0).buttons = 0x1;              // keep the pad "active"
        const gf::GamePadState pad = touch.route(devices, 0);
        check(pad.stick_x < -50, "negative deflection preserved");
        ++cases;
    }

    // 8. Quit and pointer are reported without inventing buttons.
    {
        gf::InputSnapshot devices;
        devices.quit = true;
        devices.pointer.x = 300;
        devices.pointer.left = true;
        const gf::GamePadState pad = router.route(devices, 0);
        check(pad.quit_requested, "quit is forwarded");
        check(!pad.a && !pad.b, "quit does not fabricate buttons");
        ++cases;
    }

    // 9. A mobile backend and a desktop backend producing the same snapshot are
    //    indistinguishable above the seam.
    {
        gf::headless::Bundle mobile, desktop;
        gf::InputSnapshot touch_only;
        gf::InputRouter mobile_router;
        mobile_router.set_touch_layout(phone_layout());
        check(touch_only.push_touch(1, 70, 410), "mobile touch");

        mobile.input().set_staged(touch_only);
        gf::InputSnapshot keyboard;
        keyboard.pad(0).buttons = 0x1;
        desktop.input().set_staged(keyboard);

        mobile.input().pump();
        desktop.input().pump();
        const gf::GamePadState from_touch = mobile_router.route(mobile.input().snapshot(), 0);
        const gf::GamePadState from_keyboard = mobile_router.route(desktop.input().snapshot(), 0);
        check(from_touch.a && from_keyboard.a, "both devices press A");
        ++cases;
    }

    // 10. Routing is deterministic for identical snapshots.
    {
        gf::InputRouter touch;
        touch.set_touch_layout(phone_layout());
        gf::InputSnapshot devices;
        devices.pad(0).axis[0] = 12345;
        check(devices.push_touch(7, 200, 260), "touch");
        check(touch.route(devices, 0) == touch.route(devices, 0), "route is pure");
        ++cases;
    }

    std::printf("{\"router_cases\":%d,\"sources\":[\"keyboard\",\"mouse\",\"touch\",\"controller\"],"
                "\"priority\":\"pad_over_touch\",\"all_passed\":true}\n", cases);
    return 0;
}
