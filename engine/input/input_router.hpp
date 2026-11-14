#pragma once
// Input router: one snapshot from any mix of devices -> per-port game input.
//
// This is where "what the device reports" becomes "what the game means". It is
// portable base code, so a phone with touch, a desktop with keyboard+mouse and a
// console with four pads all reach the simulation through the same path.
//
// Sources are merged with a fixed priority per port:
//   1. Physical pad      (controller / keyboard-mapped pad from the backend)
//   2. Touch overlay     (virtual stick and buttons, only if configured)
// Nothing here reads a device directly or knows what a platform is.
#include "engine/input/input_map.hpp"
#include "engine/input/input_snapshot.hpp"

namespace gf {

// A rectangle in reference-resolution space where a virtual button lives.
struct TouchButton {
    s32 x = 0, y = 0, width = 0, height = 0;
    u32 raw_button = 0;    // device bit this rectangle stands in for
};

// Virtual analogue stick: touches inside the circle drive the stick.
struct TouchStick {
    s32 center_x = 0, center_y = 0, radius = 0;
    u32 axis_x = 0, axis_y = 1;   // indices into RawInputState::axis
};

struct TouchLayout {
    static constexpr unsigned kMaxButtons = 12;
    bool enabled = false;
    TouchButton buttons[kMaxButtons]{};
    unsigned button_count = 0;
    bool has_stick = false;
    TouchStick stick{};
    bool invert_stick_y = false;

    bool add_button(const TouchButton& button) {
        if (button_count >= kMaxButtons) return false;
        buttons[button_count++] = button;
        return true;
    }
};

// Per-port configuration: which device pad feeds it and which map converts it.
struct PortBinding {
    unsigned device_pad = 0;     // index into InputSnapshot::pads
    InputMap map = default_input_map();
};

class InputRouter {
public:
    static constexpr unsigned kMaxPorts = InputSnapshot::kMaxPorts;
    InputRouter(){for(unsigned port=0;port<kMaxPorts;++port)ports_[port].device_pad=port;}

    void set_port(unsigned port, const PortBinding& binding) {
        if (port < kMaxPorts) ports_[port] = binding;
    }
    const PortBinding& port(unsigned index) const { return ports_[index < kMaxPorts ? index : 0]; }

    void set_touch_layout(const TouchLayout& layout) { touch_ = layout; }
    const TouchLayout& touch_layout() const { return touch_; }

    // Priority: a physical pad that reports any activity wins; otherwise the
    // touch overlay is consulted. This keeps a connected controller usable on a
    // phone without the overlay fighting it.
    GamePadState route(const InputSnapshot& snapshot, unsigned port) const {
        const PortBinding& binding = this->port(port);
        RawInputState raw = snapshot.pad(binding.device_pad);
        raw.quit = raw.quit || snapshot.quit;

        GamePadState state = apply(binding.map, raw);
        state.quit_requested = raw.quit;

        if (touch_.enabled && !pad_active(raw)) {
            GamePadState overlay = touch_state(snapshot, binding.map);
            state = merge(state, overlay);
        }
        return state;
    }

private:
    static bool pad_active(const RawInputState& raw) {
        if (raw.buttons) return true;
        for (s16 value : raw.axis) {
            // A deliberate small threshold: noise must not disable the overlay.
            if (value > 4096 || value < -4096) return true;
        }
        return false;
    }

    // Builds a synthetic device state from active touches, then runs it through
    // the same map so deadzones and scaling stay consistent.
    GamePadState touch_state(const InputSnapshot& snapshot, const InputMap& map) const {
        RawInputState raw;
        for (unsigned i = 0; i < snapshot.touch_count; ++i) {
            const TouchPoint& point = snapshot.touches[i];
            for (unsigned b = 0; b < touch_.button_count; ++b) {
                const TouchButton& button = touch_.buttons[b];
                if (point.x >= button.x && point.x < button.x + button.width &&
                    point.y >= button.y && point.y < button.y + button.height) {
                    raw.buttons |= button.raw_button;
                }
            }
            if (touch_.has_stick && touch_.stick.radius > 0) {
                const s32 dx = point.x - touch_.stick.center_x;
                const s32 dy = point.y - touch_.stick.center_y;
                const s64 distance_sq = s64(dx) * dx + s64(dy) * dy;
                const s64 radius_sq = s64(touch_.stick.radius) * touch_.stick.radius;
                if (distance_sq <= radius_sq) {
                    // Normalise to full-scale device units so the map's deadzone
                    // applies unchanged.
                    s32 nx = s32(s64(dx) * 32767 / touch_.stick.radius);
                    s32 ny = s32(s64(dy) * 32767 / touch_.stick.radius);
                    if (nx > 32767) nx = 32767;
                    if (nx < -32768) nx = -32768;
                    if (ny > 32767) ny = 32767;
                    if (ny < -32768) ny = -32768;
                    if (touch_.invert_stick_y) ny = -ny;
                    if (touch_.stick.axis_x < 6) raw.axis[touch_.stick.axis_x] = s16(nx);
                    if (touch_.stick.axis_y < 6) raw.axis[touch_.stick.axis_y] = s16(ny);
                }
            }
        }
        return apply(map, raw);
    }

    // Union of digital buttons, max of analogue magnitudes. A physical pad's
    // value is never reduced by the overlay.
    static GamePadState merge(const GamePadState& base, const GamePadState& overlay) {
        GamePadState out = base;
        out.a = out.a || overlay.a;
        out.b = out.b || overlay.b;
        out.x = out.x || overlay.x;
        out.y = out.y || overlay.y;
        out.start = out.start || overlay.start;
        out.dpad_up = out.dpad_up || overlay.dpad_up;
        out.dpad_down = out.dpad_down || overlay.dpad_down;
        out.dpad_left = out.dpad_left || overlay.dpad_left;
        out.dpad_right = out.dpad_right || overlay.dpad_right;
        out.l = out.l || overlay.l;
        out.r = out.r || overlay.r;
        out.z = out.z || overlay.z;
        out.trigger_l_pressed = out.trigger_l_pressed || overlay.trigger_l_pressed;
        out.trigger_r_pressed = out.trigger_r_pressed || overlay.trigger_r_pressed;
        out.stick_x = larger_mag(out.stick_x, overlay.stick_x);
        out.stick_y = larger_mag(out.stick_y, overlay.stick_y);
        out.cstick_x = larger_mag(out.cstick_x, overlay.cstick_x);
        out.cstick_y = larger_mag(out.cstick_y, overlay.cstick_y);
        out.trigger_l = larger_mag(out.trigger_l, overlay.trigger_l);
        out.trigger_r = larger_mag(out.trigger_r, overlay.trigger_r);
        return out;
    }
    // Analogue merge keeps whichever source deflected further, preserving sign;
    // a plain max() would discard a large negative deflection.
    static s32 larger_mag(s32 a, s32 b) {
        const s32 mag_a = a < 0 ? -a : a;
        const s32 mag_b = b < 0 ? -b : b;
        return mag_b > mag_a ? b : a;
    }

    PortBinding ports_[kMaxPorts]{};
    TouchLayout touch_{};
};

} // namespace gf
