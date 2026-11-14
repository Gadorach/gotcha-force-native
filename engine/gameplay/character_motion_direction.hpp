#pragma once
#include <array>
#include "engine/core/types.hpp"
namespace gf {
struct CharacterMotionTarget {u8 group=0,index=0;};
struct CharacterMotionDirection {
    CharacterMotionTarget centre,side;
    f32 side_weight=0;
};
// Source80049184: controls are three group bytes followed by three indices.
// Source1DFC is signed16 (PSQ W1/GQR5, signed16/scale0). Its divisor is exactly
// 16384, NOT 32768. Numeric selection only: no clip/model/actor admission.
// Zero uses the nonnegative side. -32768 produces2; do not clamp or infer that
// the existing [0,1] pose interpolation path admits extrapolation.
inline CharacterMotionDirection character_motion_direction(const std::array<u8,6>& controls,s16 steering){
    const unsigned side=steering<0?2:0;
    const s32 magnitude=steering<0?-s32(steering):s32(steering);
    return {{controls[1],controls[4]},{controls[side],controls[side+3]},f32(magnitude)/16384.0f};
}
} // namespace gf
