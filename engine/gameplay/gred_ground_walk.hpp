#pragma once
#include "engine/gameplay/character_movement_heading.hpp"
namespace gf {
struct GRedGroundWalkEntry {CharacterMovementTurn turn;f32 speed44=0;s16 travel5ae=0;u8 blend_duration=0;};
inline bool gred_ground_walk_entry(const CharacterMovementProfile& p,const CharacterMovementHeading& heading,
                                  const CharacterMovementTurn& current,GRedGroundWalkEntry* out){
    if(!out||!character_movement_profile_valid(p)||!p.walk_operands||p.model!=0x615||p.movement_type||p.class_flags!=12||!heading.has_direction)return false;
    const s32 magnitude=heading.delta5aa<0?-s32(heading.delta5aa):s32(heading.delta5aa);
    const f32 duration=std::fma(f32(f32(magnitude)/p.walk_angle_divisor),f32(s32(p.turn_counts[4])-p.turn_counts[0]),f32(p.turn_counts[0]));
    if(!std::isfinite(duration)||duration<0||duration>255)return false;
    GRedGroundWalkEntry entry;entry.turn=current;entry.turn.delta5aa=heading.delta5aa;
    entry.blend_duration=u8(duration);entry.travel5ae=heading.desired5ac;entry.speed44=p.walk_speeds[source_heading_sector(heading.delta5aa,1)];
    if(!character_movement_turn_begin(entry.turn,entry.blend_duration,&entry.turn))return false;
    *out=entry;return true;
}
//66E40 ordinary walk: source multiplication order and signed16 stores.
inline bool gred_ground_walk_steer(const CharacterMovementProfile& p,const CharacterMovementTurn& input,s16 delta5aa,
                                  f32 rate664,f32 delta1dc8,CharacterMovementTurn* out){
    if(!out||!character_movement_profile_valid(p)||!p.walk_operands||p.model!=0x615||p.movement_type||p.class_flags!=12||
       !std::isfinite(rate664)||rate664<0||rate664>16||!std::isfinite(delta1dc8)||delta1dc8<0||delta1dc8>16)return false;
    return character_movement_turn_steer(p,input,delta5aa,rate664,delta1dc8,0,out);
}
inline bool gred_ground_stop_speed(const CharacterMovementProfile& p,f32 speed,f32 delta,f32* out){
    if(!out||!character_movement_profile_valid(p)||!p.walk_operands||p.model!=0x615||p.movement_type||p.class_flags!=12||
       !std::isfinite(speed)||std::abs(speed)>1e5f||!std::isfinite(delta)||delta<0||delta>16)return false;
    const f32 result=f32(speed*std::fma(p.stop_damping_gain,f32(1-delta),p.stop_damping_base));
    if(!std::isfinite(result)||std::abs(result)>1e5f)return false;
    *out=result;return true;
}
}
