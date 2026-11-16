#pragma once
#include <algorithm>
#include "engine/gameplay/battle_swept_circle.hpp"
#include "engine/gameplay/character_movement_heading.hpp"
namespace gf {
//Camera-relative INPUT, locomotion/body facing and upper-body attack aim are
//separate consumers. The camera must never assign the body's orientation.
struct NativeBorgMovementFrame {BattlePoint forward{0,-1},right{1,0};};
inline bool native_borg_movement_frame(BattlePoint forward,NativeBorgMovementFrame* out){
    const f32 length=std::hypot(forward.x,forward.z);if(!out||!std::isfinite(length)||length<1e-5f)return false;
    forward.x/=length;forward.z/=length;*out={forward,{-forward.z,forward.x}};return true;
}
inline bool native_borg_movement_frame_valid(const NativeBorgMovementFrame& frame){
    for(f32 x:{frame.forward.x,frame.forward.z,frame.right.x,frame.right.z})if(!std::isfinite(x))return false;
    const auto f=frame.forward,r=frame.right;
    return std::abs(f.x*f.x+f.z*f.z-1)<.001f&&std::abs(r.x*r.x+r.z*r.z-1)<.001f&&std::abs(f.x*r.x+f.z*r.z)<.001f;
}
inline BattlePoint native_borg_move_direction(f32 x,f32 y,const NativeBorgMovementFrame& frame){
    const f32 length=std::hypot(x,y);if(length>1){x/=length;y/=length;}
    return {frame.right.x*x-frame.forward.x*y,frame.right.z*x-frame.forward.z*y};
}
inline BattlePoint native_borg_direction(BattlePoint from,BattlePoint to,BattlePoint fallback={0,-1}){
    const f32 x=to.x-from.x,z=to.z-from.z,length=std::hypot(x,z);
    return std::isfinite(length)&&length>1e-5f?BattlePoint{x/length,z/length}:fallback;
}
inline f32 native_borg_aim_delta(BattlePoint body,BattlePoint aim){
    return std::remainder(std::atan2(aim.x,aim.z)-std::atan2(body.x,body.z),6.28318530718f);
}
inline BattlePoint native_borg_velocity(BattlePoint current,BattlePoint desired,f32 maximum_change){
    const f32 x=desired.x-current.x,z=desired.z-current.z,length=std::hypot(x,z);
    if(length<=maximum_change||length==0)return desired;
    return {current.x+x*maximum_change/length,current.z+z*maximum_change/length};
}
inline BattlePoint native_borg_turn(BattlePoint current,BattlePoint desired,f32 maximum_radians){
    if(desired.x==0&&desired.z==0)return current;
    const f32 from=std::atan2(current.x,current.z),to=std::atan2(desired.x,desired.z);
    const f32 delta=std::remainder(to-from,6.28318530718f),angle=from+std::clamp(delta,-maximum_radians,maximum_radians);
    return {std::sin(angle),std::cos(angle)};
}
//Native-camera -> source-angle observation; never assigns Borg body facing.
inline bool native_borg_source_heading_frame(const NativeBorgMovementFrame& input,BattlePoint body,CharacterMovementHeadingFrame* out){
    if(!out||!native_borg_movement_frame_valid(input)||std::abs(input.forward.x*input.right.z-input.forward.z*input.right.x-1)>.001f||
       !std::isfinite(body.x)||!std::isfinite(body.z)||std::hypot(body.x,body.z)<1e-5f)return false;
    s16 forward=0,heading=0;
    if(!source_heading_s16(input.forward.x,input.forward.z,&forward)||!source_heading_s16(body.x,body.z,&heading))return false;
    *out={heading,source_heading_wrap(s32(forward)+0x8000),0,false,false};return true;
}
}
