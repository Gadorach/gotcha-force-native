#pragma once
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
struct CharacterAirResourceState {s16 value736=0,delay73a=0;u8 air_actions747=0;};
inline s16 character_air_resource_narrow(s32 value){return std::bit_cast<s16>(u16(value));}
//Shared5568C resource branch. Cached87 is the PREVIOUS accepted post-update's
//airborne bit, not a newly sampled grounded flag or an animation classification.
inline bool character_air_resource_recover(const CharacterAirResourceState& input,u8 movement_type,u32 flags5e0,u8 cached87,
                                           s16 limit,s16 increment,CharacterAirResourceState* out){
    if(!out)return false;
    auto s=input;const bool special=movement_type==2||movement_type==3||movement_type==6;
    if((special&&!(flags5e0&0x80u))||(!special&&!cached87)){
        if(!s.delay73a){s.value736=character_air_resource_narrow(s32(s.value736)+increment);if(s.value736>limit)s.value736=limit;}
        else s.delay73a=character_air_resource_narrow(s32(s.delay73a)-1);
        if(!special)s.air_actions747=0;
    }
    *out=s;return true;
}
//Shared664A0: fused float subtraction -> signed word conversion -> signed16
//store -> negative clamp. This is not the count-based jump-gauge admission rule.
inline bool character_air_resource_consume(const CharacterAirResourceState& input,s16 cost,s16 recovery_delay,f32 rate5f4,CharacterAirResourceState* out){
    if(!out)return false;
    auto s=input;
    if(s.value736>0){
        if(cost<0||recovery_delay<0||!std::isfinite(rate5f4)||rate5f4<0||rate5f4>16)return false;
        const f32 candidate=std::fma(-f32(cost),rate5f4,f32(s.value736));
        s.value736=character_air_resource_narrow(s32(candidate));s.delay73a=recovery_delay;
        if(s.value736<0)s.value736=0;
    }
    *out=s;return true;
}
}
