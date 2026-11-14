#pragma once
// Raw device input -> game-facing pad state.
//
// A platform backend reports only device bits and raw s16 axes (see
// `IInputBackend::poll` / `RawInputState`). This header is the one place that
// knows what those bits *mean* to the game, so adding a platform never means
// relearning game semantics and remapping a controller never touches gameplay
// code.
//
// Units and conventions:
//   * Sticks use s8-style units: every stick axis is in [-128, 127]. 0 is
//     centred; full positive deflection saturates at +127 and full negative at
//     -128 (the two's-complement asymmetry is deliberate, see kPadAxisScale).
//   * Triggers are 0..127, where 0 is fully released.
//   * Device sticks are full-scale s16 (-32768..32767). Device triggers are
//     0..32767, which is the range SDL's trigger axes report.
//   * `deadzone` is a fraction of full stick travel in [0, 1] and is applied
//     per axis, not radially: a diagonal push is never silently reshaped.
//   * Outside the deadzone the value is rescaled back to full travel, so the
//     transfer curve is continuous: inside the deadzone the result is exactly
//     0, and just outside it is a small non-zero step with no jump. Because the
//     output is an integer, "small" is one game unit (1/128 of full travel);
//     that single unit is the smallest representable step.
//   * `invert_y` flips both Y axes (stick and c-stick). X axes are untouched.
//   * One `InputMap` describes one port's device layout. Ports differ only by
//     which map the caller hands to `apply`; `apply` takes no port argument.
//
// Everything here is pure: no allocation, no I/O, no globals, no static mutable
// state, and the same input always produces byte-identical output.
#include "engine/core/backends.hpp" // RawInputState
#include "engine/core/types.hpp"

namespace gf {

// Game pad axis limits and scale. Values are scaled by 128 and then clamped, so
// the positive side saturates at +127 and the negative side can reach -128.
inline constexpr s32 kPadAxisMin = -128;
inline constexpr s32 kPadAxisMax = 127;
inline constexpr s32 kPadAxisScale = 128;
inline constexpr s32 kPadTriggerMin = 0;
inline constexpr s32 kPadTriggerMax = 127;

// Full-scale range of the device axes carried by RawInputState. Negative s16
// values reach one count further than positive ones, so the two sides are
// normalised separately to land exactly on ±1.0.
inline constexpr s32 kDeviceAxisPositiveMax = 32767;
inline constexpr s32 kDeviceAxisNegativeMax = 32768;
inline constexpr s32 kDeviceTriggerMax = 32767;

// Number of RawInputState::axis entries. Indices outside this range fail closed
// to a neutral 0 instead of reading out of bounds.
inline constexpr unsigned kRawAxisCount = 6;

static_assert(sizeof(RawInputState::axis) / sizeof(s16) == kRawAxisCount,
              "RawInputState::axis layout changed; update the axis indices");

// Game-facing pad state. Buttons are independent booleans (not bit flags) so a
// remap can never merge two game buttons by accident.
struct GamePadState {
    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;
    bool start = false;
    bool dpad_up = false;
    bool dpad_down = false;
    bool dpad_left = false;
    bool dpad_right = false;
    bool l = false;
    bool r = false;
    bool z = false;

    // Sticks in [-128, 127]; 0 is centred.
    s32 stick_x = 0;
    s32 stick_y = 0;
    s32 cstick_x = 0;
    s32 cstick_y = 0;

    // Analogue triggers in [0, 127]; 0 is fully released.
    s32 trigger_l = 0;
    s32 trigger_r = 0;

    // Digital view of the same triggers, using InputMap::trigger_threshold.
    bool trigger_l_pressed = false;
    bool trigger_r_pressed = false;

    // Platform quit request (window closed, Escape...) passed straight through.
    bool quit_requested = false;

    friend bool operator==(const GamePadState&, const GamePadState&) = default;
};

// Per-port device layout. Defaults match the SDL backend's documented bit
// layout in platform/pc/sdl_platform.cpp; a different device only needs a
// different InputMap, never a change to `apply`.
struct InputMap {
    // Device button bits. A zero mask disables the button.
    u32 mask_a = 1u << 0;
    u32 mask_b = 1u << 1;
    u32 mask_x = 1u << 2;
    u32 mask_y = 1u << 3;
    u32 mask_start = 1u << 4;
    u32 mask_dpad_up = 1u << 5;
    u32 mask_dpad_down = 1u << 6;
    u32 mask_dpad_left = 1u << 7;
    u32 mask_dpad_right = 1u << 8;
    u32 mask_l = 1u << 9;
    u32 mask_r = 1u << 10;
    u32 mask_z = 1u << 11;

    // RawInputState::axis indices.
    u8 axis_stick_x = 0;
    u8 axis_stick_y = 1;
    u8 axis_cstick_x = 2;
    u8 axis_cstick_y = 3;
    u8 axis_trigger_l = 4;
    u8 axis_trigger_r = 5;

    // Fraction of full stick travel ignored around centre, in [0, 1].
    f32 deadzone = 0.15f;
    // Fraction of full trigger travel that counts as a digital press, in [0, 1].
    f32 trigger_threshold = 0.5f;
    // Flip both Y axes (stick and c-stick).
    bool invert_y = false;
};

// Default map: identical to the SDL backend's layout.
constexpr InputMap default_input_map() { return InputMap{}; }

namespace input_detail {

// NaN-safe: anything not strictly above 0 clamps to 0, so a corrupt map can
// never feed a NaN into the integer quantiser.
constexpr f32 clamp_unit(f32 v) { return v > 1.0f ? 1.0f : (v > 0.0f ? v : 0.0f); }

constexpr s32 clamp_s32(s32 v, s32 lo, s32 hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Deterministic round-half-away-from-zero. Avoids <cmath> and any dependence on
// the host's current floating-point rounding mode.
constexpr s32 round_to_s32(f32 v) {
    return v >= 0.0f ? static_cast<s32>(v + 0.5f) : static_cast<s32>(v - 0.5f);
}

inline s16 raw_axis(const RawInputState& raw, u8 index) {
    return index < kRawAxisCount ? raw.axis[index] : s16{0};
}

// Normalise a signed stick axis to [-1, 1], using each side's own extent so the
// extremes land exactly on ±1.
inline f32 normalize_axis(s16 raw) {
    if (raw < 0) return f32(raw) / f32(kDeviceAxisNegativeMax);
    return f32(raw) / f32(kDeviceAxisPositiveMax);
}

// Normalise a trigger axis to [0, 1]. Negative device values are treated as
// released rather than as a second direction.
inline f32 normalize_trigger(s16 raw) {
    if (raw <= 0) return 0.0f;
    const s32 v = raw > kDeviceTriggerMax ? kDeviceTriggerMax : s32(raw);
    return f32(v) / f32(kDeviceTriggerMax);
}

inline s32 map_axis(s16 raw, f32 deadzone, bool invert) {
    const f32 dz = clamp_unit(deadzone);
    f32 n = normalize_axis(raw);
    if (invert) n = -n;
    const f32 magnitude = n < 0.0f ? -n : n;
    if (magnitude <= dz || dz >= 1.0f) return 0;
    // Rescale [dz, 1] onto [0, 1] so crossing the deadzone is continuous.
    const f32 scaled = (magnitude - dz) / (1.0f - dz);
    const s32 quantised = clamp_s32(round_to_s32(scaled * f32(kPadAxisScale)),
                                    0, kPadAxisScale);
    // Strictly outside the deadzone must never quantise back to 0.
    const s32 magnitude_out = quantised == 0 ? 1 : quantised;
    const s32 signed_out = n < 0.0f ? -magnitude_out : magnitude_out;
    return clamp_s32(signed_out, kPadAxisMin, kPadAxisMax);
}

inline s32 map_trigger(s16 raw) {
    return clamp_s32(round_to_s32(normalize_trigger(raw) * f32(kPadTriggerMax)),
                     kPadTriggerMin, kPadTriggerMax);
}

inline bool trigger_pressed(s16 raw, f32 threshold) {
    const f32 n = normalize_trigger(raw);
    // A threshold of 0 means "any non-zero travel", never "always pressed".
    return n > 0.0f && n >= clamp_unit(threshold);
}

} // namespace input_detail

// Convert one port's raw device state into game pad state. Pure and
// deterministic; never allocates and never reads anything outside its arguments.
[[nodiscard]] inline GamePadState apply(const InputMap& map, const RawInputState& raw) {
    const f32 deadzone = input_detail::clamp_unit(map.deadzone);
    const f32 threshold = input_detail::clamp_unit(map.trigger_threshold);

    GamePadState pad;
    pad.a = (raw.buttons & map.mask_a) != 0;
    pad.b = (raw.buttons & map.mask_b) != 0;
    pad.x = (raw.buttons & map.mask_x) != 0;
    pad.y = (raw.buttons & map.mask_y) != 0;
    pad.start = (raw.buttons & map.mask_start) != 0;
    pad.dpad_up = (raw.buttons & map.mask_dpad_up) != 0;
    pad.dpad_down = (raw.buttons & map.mask_dpad_down) != 0;
    pad.dpad_left = (raw.buttons & map.mask_dpad_left) != 0;
    pad.dpad_right = (raw.buttons & map.mask_dpad_right) != 0;
    pad.l = (raw.buttons & map.mask_l) != 0;
    pad.r = (raw.buttons & map.mask_r) != 0;
    pad.z = (raw.buttons & map.mask_z) != 0;

    const s16 device_stick_x = input_detail::raw_axis(raw, map.axis_stick_x);
    const s16 device_stick_y = input_detail::raw_axis(raw, map.axis_stick_y);
    const s16 device_cstick_x = input_detail::raw_axis(raw, map.axis_cstick_x);
    const s16 device_cstick_y = input_detail::raw_axis(raw, map.axis_cstick_y);
    const s16 device_trigger_l = input_detail::raw_axis(raw, map.axis_trigger_l);
    const s16 device_trigger_r = input_detail::raw_axis(raw, map.axis_trigger_r);

    pad.stick_x = input_detail::map_axis(device_stick_x, deadzone, false);
    pad.stick_y = input_detail::map_axis(device_stick_y, deadzone, map.invert_y);
    pad.cstick_x = input_detail::map_axis(device_cstick_x, deadzone, false);
    pad.cstick_y = input_detail::map_axis(device_cstick_y, deadzone, map.invert_y);

    pad.trigger_l = input_detail::map_trigger(device_trigger_l);
    pad.trigger_r = input_detail::map_trigger(device_trigger_r);
    pad.trigger_l_pressed = input_detail::trigger_pressed(device_trigger_l, threshold);
    pad.trigger_r_pressed = input_detail::trigger_pressed(device_trigger_r, threshold);

    pad.quit_requested = raw.quit;
    return pad;
}

} // namespace gf
