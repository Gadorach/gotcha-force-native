// Input-map tests: raw device bits/axes must become the game pad state that
// gameplay code expects, with no platform semantics leaking out of the map.
// No SDL, GPU, emulator or game data involved.
#include "engine/input/input_map.hpp"

#include <cstdio>
#include <cstdlib>

namespace {

constexpr int kButtonCount = 12;

int count_buttons(const gf::GamePadState& pad) {
    return int(pad.a) + int(pad.b) + int(pad.x) + int(pad.y) + int(pad.start) +
           int(pad.dpad_up) + int(pad.dpad_down) + int(pad.dpad_left) +
           int(pad.dpad_right) + int(pad.l) + int(pad.r) + int(pad.z);
}

struct ButtonCase {
    const char* name;
    gf::u32 mask;
    bool gf::GamePadState::* member;
};

constexpr ButtonCase kButtons[kButtonCount] = {
    {"A", 1u << 0, &gf::GamePadState::a},
    {"B", 1u << 1, &gf::GamePadState::b},
    {"X", 1u << 2, &gf::GamePadState::x},
    {"Y", 1u << 3, &gf::GamePadState::y},
    {"Start", 1u << 4, &gf::GamePadState::start},
    {"DpadUp", 1u << 5, &gf::GamePadState::dpad_up},
    {"DpadDown", 1u << 6, &gf::GamePadState::dpad_down},
    {"DpadLeft", 1u << 7, &gf::GamePadState::dpad_left},
    {"DpadRight", 1u << 8, &gf::GamePadState::dpad_right},
    {"L", 1u << 9, &gf::GamePadState::l},
    {"R", 1u << 10, &gf::GamePadState::r},
    {"Z", 1u << 11, &gf::GamePadState::z},
};

} // namespace

int main() {
    int cases = 0;
    const auto check = [&cases](bool ok, const char* what) {
        ++cases;
        if (!ok) {
            std::fprintf(stderr, "input map failure: %s\n", what);
            std::abort();
        }
    };

    // ---------------------------------------------------------------- buttons
    const gf::InputMap def = gf::default_input_map();
    check(def.mask_a == (1u << 0) && def.mask_b == (1u << 1) && def.mask_x == (1u << 2) &&
              def.mask_y == (1u << 3) && def.mask_start == (1u << 4) &&
              def.mask_dpad_up == (1u << 5) && def.mask_dpad_down == (1u << 6) &&
              def.mask_dpad_left == (1u << 7) && def.mask_dpad_right == (1u << 8) &&
              def.mask_l == (1u << 9) && def.mask_r == (1u << 10) &&
              def.mask_z == (1u << 11),
          "default masks match the documented SDL device layout");

    for (const ButtonCase& bc : kButtons) {
        gf::RawInputState raw{};
        raw.buttons = bc.mask;
        const gf::GamePadState pad = gf::apply(def, raw);
        check(pad.*(bc.member), "device bit sets its game button");
        check(count_buttons(pad) == 1, "a single device bit sets exactly one button");
        check(!pad.quit_requested, "buttons do not raise quit");
    }

    {
        gf::RawInputState raw{};
        raw.buttons = 0xFFFFFFFFu;
        check(count_buttons(gf::apply(def, raw)) == kButtonCount,
              "all mapped bits set every button and extras are ignored");
    }

    {
        // Bits outside the documented map must not create phantom buttons.
        const gf::u32 ignored[] = {1u << 12, 1u << 13, 1u << 31};
        for (gf::u32 bit : ignored) {
            gf::RawInputState raw{};
            raw.buttons = bit;
            const gf::GamePadState pad = gf::apply(def, raw);
            check(count_buttons(pad) == 0, "unmapped device bit is ignored");
        }
    }

    {
        // Remapping a mask really drives the mapping (it is not hardcoded).
        gf::InputMap remapped = def;
        remapped.mask_a = 1u << 12;
        gf::RawInputState raw{};
        raw.buttons = 1u << 12;
        check(gf::apply(remapped, raw).a, "remapped mask selects its button");
        raw.buttons = 1u << 0;
        check(!gf::apply(remapped, raw).a, "old bit no longer matches after a remap");
    }

    {
        // A zero mask disables a button instead of matching nothing unexpectedly.
        gf::InputMap disabled = def;
        disabled.mask_z = 0;
        gf::RawInputState raw{};
        raw.buttons = 0xFFFFFFFFu;
        check(!gf::apply(disabled, raw).z, "zero mask disables the button");
    }

    // ---------------------------------------------------------------- deadzone
    {
        gf::InputMap map = def;
        map.deadzone = 0.5f;
        gf::RawInputState raw{};
        raw.axis[0] = 16000; // ~0.488 of full travel: inside
        check(gf::apply(map, raw).stick_x == 0, "axis inside the deadzone is exactly zero");
        raw.axis[0] = 16383; // ~0.49997: still inside
        check(gf::apply(map, raw).stick_x == 0, "axis just inside the deadzone is zero");
        raw.axis[0] = 18022; // ~0.550: just outside
        const gf::s32 outside = gf::apply(map, raw).stick_x;
        check(outside > 0, "axis just outside the deadzone is non-zero");
        check(outside <= 32, "axis just outside the deadzone stays a small step");
        raw.axis[0] = -16383;
        check(gf::apply(map, raw).stick_x == 0, "negative axis inside the deadzone is zero");
        raw.axis[0] = -18022;
        check(gf::apply(map, raw).stick_x < 0, "negative axis just outside the deadzone is non-zero");
    }

    {
        // The first non-zero sample past the edge must be the smallest step.
        gf::InputMap map = def; // default deadzone 0.15
        gf::RawInputState raw{};
        raw.axis[0] = 5000; // ~0.1526, just past 0.15
        check(gf::apply(map, raw).stick_x == 1, "first movement past the deadzone is one unit");
    }

    {
        // Every sample strictly inside the deadzone maps to exactly 0.
        gf::InputMap map = def;
        bool all_zero = true;
        for (gf::s32 v = -4900; v <= 4900; v += 100) {
            gf::RawInputState raw{};
            raw.axis[0] = gf::s16(v);
            if (gf::apply(map, raw).stick_x != 0) all_zero = false;
        }
        check(all_zero, "the whole interior of the deadzone maps to zero");
    }

    {
        // Degenerate deadzones must be safe and meaningful.
        gf::RawInputState raw{};
        raw.axis[0] = 32767;
        gf::InputMap none = def;
        none.deadzone = 0.0f;
        check(gf::apply(none, raw).stick_x == gf::kPadAxisMax, "zero deadzone keeps full range");
        gf::InputMap all = def;
        all.deadzone = 1.0f;
        check(gf::apply(all, raw).stick_x == 0, "deadzone of one silences the axis");
        gf::InputMap over = def;
        over.deadzone = 5.0f;
        check(gf::apply(over, raw).stick_x == 0, "out-of-range deadzone is clamped");
    }

    // ------------------------------------------------------------ continuity
    {
        // Sweep the whole positive half: monotonic, no jump, no oscillation.
        gf::InputMap map = def;
        gf::s32 prev = 0;
        bool first = true;
        bool saw_motion = false;
        gf::s32 first_step = 0;
        for (gf::s32 v = 0; v <= 32767; v += 64) {
            gf::RawInputState raw{};
            raw.axis[0] = gf::s16(v);
            const gf::s32 out = gf::apply(map, raw).stick_x;
            if (!first) {
                check(out >= prev, "positive axis sweep is monotonic");
                check(out - prev <= 2, "positive axis sweep has no jump");
            }
            if (out != 0 && !saw_motion) {
                saw_motion = true;
                first_step = out;
            }
            first = false;
            prev = out;
        }
        check(saw_motion, "positive sweep eventually moves");
        check(first_step <= 2, "positive sweep starts from the smallest step");

        prev = 0;
        first = true;
        saw_motion = false;
        first_step = 0;
        for (gf::s32 v = 0; v >= -32768; v -= 64) {
            gf::RawInputState raw{};
            raw.axis[0] = gf::s16(v);
            const gf::s32 out = gf::apply(map, raw).stick_x;
            if (!first) {
                check(out <= prev, "negative axis sweep is monotonic");
                check(prev - out <= 2, "negative axis sweep has no jump");
            }
            if (out != 0 && !saw_motion) {
                saw_motion = true;
                first_step = -out;
            }
            first = false;
            prev = out;
        }
        check(saw_motion, "negative sweep eventually moves");
        check(first_step <= 2, "negative sweep starts from the smallest step");
    }

    // -------------------------------------------------------- full deflection
    {
        gf::RawInputState raw{};
        raw.axis[0] = 32767;
        check(gf::apply(def, raw).stick_x == gf::kPadAxisMax,
              "full positive deflection clamps to kPadAxisMax");
        raw.axis[0] = -32768;
        check(gf::apply(def, raw).stick_x == gf::kPadAxisMin,
              "full negative deflection clamps to kPadAxisMin");
        raw.axis[0] = 32767;
        raw.axis[1] = 32767;
        raw.axis[2] = -32768;
        raw.axis[3] = -32768;
        const gf::GamePadState pad = gf::apply(def, raw);
        check(pad.stick_y == gf::kPadAxisMax && pad.cstick_y == gf::kPadAxisMin,
              "every stick axis saturates at its own extreme");
        check(pad.cstick_x == gf::kPadAxisMin, "c-stick uses its own device axis");
    }

    // -------------------------------------------------------------- negative
    {
        gf::RawInputState raw{};
        raw.axis[0] = -16384;
        const gf::s32 negative = gf::apply(def, raw).stick_x;
        check(negative < 0, "negative device axis yields negative game axis");

        gf::RawInputState mirrored{};
        mirrored.axis[0] = 16384;
        const gf::s32 positive = gf::apply(def, mirrored).stick_x;
        check(positive > 0, "positive device axis yields positive game axis");
        check(negative == -positive, "the two sides mirror in magnitude");
    }

    // ------------------------------------------------------------ Y inversion
    {
        gf::InputMap inverted = def;
        inverted.invert_y = true;
        gf::RawInputState raw{};
        raw.axis[0] = 16384;
        raw.axis[1] = 16384;
        raw.axis[3] = 16384;
        const gf::GamePadState normal = gf::apply(def, raw);
        const gf::GamePadState flipped = gf::apply(inverted, raw);
        check(normal.stick_y > 0 && normal.cstick_y > 0, "un-inverted Y is positive");
        check(flipped.stick_y == -normal.stick_y, "invert-Y flips stick Y");
        check(flipped.cstick_y == -normal.cstick_y, "invert-Y flips c-stick Y");
        check(flipped.stick_x == normal.stick_x && flipped.cstick_x == normal.cstick_x,
              "invert-Y leaves X axes untouched");
    }

    // ---------------------------------------------------------------- triggers
    {
        gf::RawInputState raw{};
        raw.axis[4] = 16000; // ~0.488 of travel
        check(!gf::apply(def, raw).trigger_l_pressed, "trigger below the threshold is not pressed");
        raw.axis[4] = 16384; // ~0.50002
        check(gf::apply(def, raw).trigger_l_pressed, "trigger at the threshold is pressed");
        raw.axis[4] = 32767;
        const gf::GamePadState full = gf::apply(def, raw);
        check(full.trigger_l_pressed && full.trigger_l == gf::kPadTriggerMax,
              "full trigger reports max analogue travel");
        raw.axis[4] = -1234;
        const gf::GamePadState released = gf::apply(def, raw);
        check(!released.trigger_l_pressed && released.trigger_l == 0,
              "negative device trigger is released, not reversed");
        raw.axis[4] = 0;
        check(!gf::apply(def, raw).trigger_l_pressed, "resting trigger is not pressed");

        raw.axis[4] = 0;
        raw.axis[5] = 32767;
        const gf::GamePadState right = gf::apply(def, raw);
        check(right.trigger_r_pressed && !right.trigger_l_pressed,
              "left and right triggers are independent");
    }

    {
        gf::InputMap hi = def;
        hi.trigger_threshold = 0.75f;
        gf::RawInputState raw{};
        raw.axis[5] = 16384; // ~0.50 < 0.75
        check(!gf::apply(hi, raw).trigger_r_pressed, "custom high threshold rejects half travel");
        raw.axis[5] = 26000; // ~0.793 > 0.75
        check(gf::apply(hi, raw).trigger_r_pressed, "custom threshold accepts travel above it");

        gf::InputMap any = def;
        any.trigger_threshold = 0.0f;
        raw.axis[5] = 0;
        check(!gf::apply(any, raw).trigger_r_pressed,
              "zero threshold means any travel, not always pressed");
        raw.axis[5] = 100;
        check(gf::apply(any, raw).trigger_r_pressed, "zero threshold accepts tiny travel");
    }

    // ------------------------------------------------------------------- quit
    {
        gf::RawInputState raw{};
        check(!gf::apply(def, raw).quit_requested, "quit defaults to false");
        raw.quit = true;
        check(gf::apply(def, raw).quit_requested, "quit is passed through");
        check(count_buttons(gf::apply(def, raw)) == 0, "quit does not fabricate buttons");
    }

    // ------------------------------------------------------------ axis remap
    {
        gf::InputMap remapped = def;
        remapped.axis_stick_x = 2;
        gf::RawInputState raw{};
        raw.axis[2] = 16384;
        check(gf::apply(remapped, raw).stick_x > 0, "axis indices are honoured");
        raw.axis[2] = 0;
        raw.axis[0] = 16384;
        check(gf::apply(remapped, raw).stick_x == 0, "the old device axis is ignored");

        gf::InputMap out_of_range = def;
        out_of_range.axis_stick_x = 200;
        raw = {};
        raw.axis[0] = 32767;
        check(gf::apply(out_of_range, raw).stick_x == 0,
              "an out-of-range axis index reads as neutral instead of out of bounds");
    }

    // ----------------------------------------------------------- determinism
    {
        gf::RawInputState raw{};
        raw.buttons = 0x0ABu;
        raw.axis[0] = 1234;
        raw.axis[1] = -5678;
        raw.axis[2] = 9101;
        raw.axis[3] = -12131;
        raw.axis[4] = 4151;
        raw.axis[5] = 16161;
        raw.quit = true;
        const gf::GamePadState first = gf::apply(def, raw);
        const gf::GamePadState second = gf::apply(def, raw);
        check(first == second, "same input produces identical output");

        gf::InputMap inverted = def;
        inverted.invert_y = true;
        check(gf::apply(inverted, raw) == gf::apply(inverted, raw),
              "determinism holds with a non-default map too");
    }

    std::printf("{\"cases\":%d,\"buttons\":%d,\"all_passed\":true}\n", cases, kButtonCount);
    return 0;
}
