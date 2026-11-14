#pragma once
#include "engine/input/input_map.hpp"
namespace gf {
inline u32 menu_stick_directions(s32 x,s32 y,s32 threshold){
    return (x < -threshold?1u:0u)|(x > threshold?2u:0u)|
           (y < -threshold?4u:0u)|(y > threshold?8u:0u);
}
// Mapping adapter, not a recovered analogue threshold. The current host/menu
// threshold is retained explicitly; original stick threshold parity is unproven.
inline u32 menu_pad_bits(const GamePadState& pad,s32 threshold){
    return (pad.dpad_left?1u:0u)|(pad.dpad_right?2u:0u)|(pad.dpad_down?4u:0u)|(pad.dpad_up?8u:0u)|
        (pad.z?0x10u:0u)|(pad.r||pad.trigger_r_pressed?0x20u:0u)|(pad.l||pad.trigger_l_pressed?0x40u:0u)|
        (pad.a?0x100u:0u)|(pad.b?0x200u:0u)|(pad.x?0x400u:0u)|(pad.y?0x800u:0u)|(pad.start?0x1000u:0u)|
        (menu_stick_directions(pad.stick_x,pad.stick_y,threshold)<<16)|
        (menu_stick_directions(pad.cstick_x,pad.cstick_y,threshold)<<20);
}
// Original 8008BAB0(flags=3): d-pad plus main-stick digital directions.
// C-stick movement can reset that port's repeat timer, but does not navigate.
inline u32 menu_direction_events(u32 events){return (events|(events>>16))&15u;}
} // namespace gf
