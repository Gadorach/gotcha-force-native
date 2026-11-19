#pragma once
// Portable input snapshot: every input source, one frame at a time.
//
// An input backend (SDL, XInput, a mobile touch layer, a replay file) reports
// *all* devices for the frame in one structure. Base code then decides what
// those devices mean to the game. Keeping the raw snapshot separate from the
// mapping is what lets a phone, a keyboard and a gamepad drive the same
// simulation without the game knowing which is present.
#include <cstddef>

#include "engine/core/types.hpp"

namespace gf {

// Device-layer pad state. `buttons` is a backend-defined bitfield: base code
// maps it to game buttons through a table, so adding a device never changes
// game logic. Axes are full-scale device units, not game units.
struct RawInputState {
    u32 buttons = 0;      // device-defined bitfield
    s16 axis[6] = {};     // stick x/y, c-stick x/y, trigger l/r
    bool quit = false;
};

// Mouse, trackpad and pointer devices. Screen coordinates, origin top-left.
struct PointerState {
    s32 x = 0;
    s32 y = 0;
    s32 wheel = 0;
    bool left = false;
    bool right = false;
    bool middle = false;
    // Edge flags, valid for one snapshot only.
    bool left_pressed = false;
    bool left_released = false;
    bool right_pressed = false;
    bool within_window = false;
};

// A single touch point. `id` is stable while the finger stays down.
struct TouchPoint {
    u32 id = 0;
    s32 x = 0;
    s32 y = 0;
};

// One frame of input across all devices.
//
// `pads` always describes controllers/keyboard as the device layer sees them.
// Touch is reported separately because its meaning is layout dependent: a phone
// has on-screen buttons, a desktop does not. `InputRouter` performs that step.
struct InputSnapshot {
    static constexpr unsigned kMaxPorts = 4;
    static constexpr unsigned kMaxTouches = 10;

    unsigned port_count = 0;
    RawInputState pads[kMaxPorts]{};
    PointerState pointer{};
    unsigned touch_count = 0;
    TouchPoint touches[kMaxTouches]{};
    bool quit = false;
    bool frame_step = false; // Host session control, not a routed game button.

    // Convenience for backends and tests.
    RawInputState& pad(unsigned index) {
        static RawInputState unused{};
        return index < kMaxPorts ? pads[index] : unused;
    }
    const RawInputState& pad(unsigned index) const {
        static const RawInputState unused{};
        return index < kMaxPorts ? pads[index] : unused;
    }
    bool push_touch(u32 id, s32 x, s32 y) {
        if (touch_count >= kMaxTouches) return false;
        touches[touch_count++] = TouchPoint{id, x, y};
        return true;
    }
    void clear() { *this = InputSnapshot{}; }
};

// Input backend seam. Backends pump their device APIs and hand base code a
// snapshot; they never interpret the buttons.
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    // Poll the OS/device layer. Must not block and must not allocate unbounded.
    virtual void pump() = 0;
    virtual const InputSnapshot& snapshot() const = 0;
    // Human-readable device description for logs, e.g. "sdl2 keyboard+mouse+pad".
    virtual const char* describe() const { return "input"; }
};

} // namespace gf
