#pragma once
#include <algorithm>
#include "engine/input/input_map.hpp"
#include "engine/gameplay/character_movement_input.hpp"
namespace gf {
struct NativeGRedGroundInput {s16 x=0,y=0;u32 held=0;};
inline bool native_gred_soft_cardinal(s16 x,s16 y){
    return (x==0&&(y==32||y==-32))||(y==0&&(x==32||x==-32));
}
//Native convenience mapping, not recovered GameCube device calibration.
//Routed stick/WASD gives source-unit deflection; D-pad/arrows use32 units
//to request walking without crossing the source flick threshold52.
inline bool native_gred_ground_input(const CharacterMovementInputProfile& profile,const GamePadState& pad,NativeGRedGroundInput* out,std::string* error){
    if(!out||!character_movement_input_profile_valid(profile)||pad.a||pad.b||pad.x||pad.l||pad.r||pad.z||
       pad.trigger_l||pad.trigger_r||pad.trigger_l_pressed||pad.trigger_r_pressed){
        if(error)*error="source ground input refused: jump/flight/combat/trigger actions are unsupported";
        return false;
    }
    NativeGRedGroundInput result;
    const s32 x=std::clamp(pad.stick_x,-128,127)+32*(s32(pad.dpad_right)-s32(pad.dpad_left));
    const s32 y=-std::clamp(pad.stick_y,-128,127)+32*(s32(pad.dpad_up)-s32(pad.dpad_down));
    result.x=s16(std::clamp(x,-128,127));result.y=s16(std::clamp(y,-128,127));u8 direction=0;
    if(!character_movement_stick_nibble(profile,result.x,result.y,profile.low_threshold,&direction))return false;
    result.held=u32(direction)<<16;*out=result;if(error)error->clear();return true;
}
}
