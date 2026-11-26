#include <cstdio>
#include <cstdlib>
#include "platform/pc/keyboard_layout.hpp"

namespace {
using namespace gf;
using namespace gf::pc;
void check(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "Keyboard layout: %s\n", why); std::abort(); }
}
}

int main() {
    RawInputState pad;
    pc::map_keyboard_pad({}, &pad);
    check(!pad.buttons && !pad.axis[0] && !pad.axis[1],
          "an empty layout leaves a pad clear");

    pc::KeyboardPad first;
    first.start = first.a = first.b = first.x = first.y = true;
    first.l = first.r = first.z = true;
    first.dpad_up = first.dpad_down = first.dpad_left = first.dpad_right = true;
    first.axis_up = first.axis_down = first.axis_left = first.axis_right = true;
    pc::map_keyboard_pad(first, &pad);
    const u16 all = kButtonStart | kButtonA | kButtonB | kButtonX | kButtonY |
                    kButtonL | kButtonR | kButtonZ | kButtonDpadUp | kButtonDpadDown |
                    kButtonDpadLeft | kButtonDpadRight;
    check(pad.buttons == all, "all buttons map");
    check(pad.axis[0] == 20000 && pad.axis[1] == 20000,
          "the last axis direction wins");

    RawInputState second;
    pc::KeyboardPad player_two;
    player_two.a = true;
    player_two.dpad_left = true;
    player_two.axis_down = true;
    pc::map_keyboard_pad(player_two, &second);
    check(second.buttons == (kButtonA | kButtonDpadLeft) && second.axis[1] == 20000 &&
              second.axis[0] == 0,
          "dpad and axis are independent");
    pc::map_keyboard_pad(player_two, nullptr);
    std::puts("Keyboard layout: two pads map as declared PASS");
}
