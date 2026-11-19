#pragma once
#include <array>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
struct CharacterAirResourceState {s16 value736=0,delay73a=0;u8 air_actions747=0;};
enum class AirMeterRecoveryGate:u8 {PreviousGroundedCache,Behavior80Clear};
// Named entries of the metered movement cost table. The qualified source
// planners still emit these slot numbers; this is not a jump-count policy.
enum class AirMeterCharge:u8 {LiftEntry,LiftUpdate,DashHandoff,DashActive};
struct AirMeterRules {
    AirMeterRecoveryGate recovery_gate;
    s16 limit,delay,increment;
    std::array<s16,4> costs;
    constexpr s16 cost(AirMeterCharge charge)const{return costs[unsigned(charge)];}
    constexpr s16 lift_cost(bool entry)const{return cost(entry?AirMeterCharge::LiftEntry:AirMeterCharge::LiftUpdate);}
    constexpr bool matches_operands(s16 source_limit,s16 source_delay,s16 source_increment,
                                   const std::array<s16,4>& source_costs)const{
        return limit==source_limit&&delay==source_delay&&increment==source_increment&&costs==source_costs;
    }
};
inline s16 character_air_resource_narrow(s32 value){return std::bit_cast<s16>(u16(value));}
//Shared5568C resource branch. Cached87 is the PREVIOUS accepted post-update's
//airborne bit, not a newly sampled grounded flag or an animation classification.
inline bool character_air_resource_recover(const CharacterAirResourceState& input,AirMeterRecoveryGate gate,u32 flags5e0,u8 cached87,
                                            s16 limit,s16 increment,CharacterAirResourceState* out){
    if(!out)return false;
    auto s=input;const bool special=gate==AirMeterRecoveryGate::Behavior80Clear;
    if((special&&!(flags5e0&0x80u))||(!special&&!cached87)){
        if(!s.delay73a){s.value736=character_air_resource_narrow(s32(s.value736)+increment);if(s.value736>limit)s.value736=limit;}
        else s.delay73a=character_air_resource_narrow(s32(s.delay73a)-1);
        if(!special)s.air_actions747=0;
    }
    *out=s;return true;
}
inline bool character_air_resource_recover(const CharacterAirResourceState& input,u8 movement_type,u32 flags5e0,u8 cached87,
                                            s16 limit,s16 increment,CharacterAirResourceState* out){
    const auto gate=movement_type==2||movement_type==3||movement_type==6?
        AirMeterRecoveryGate::Behavior80Clear:AirMeterRecoveryGate::PreviousGroundedCache;
    return character_air_resource_recover(input,gate,flags5e0,cached87,limit,increment,out);
}
inline bool character_air_resource_recover(const CharacterAirResourceState& input,const AirMeterRules& rules,u32 flags5e0,u8 cached87,
                                            CharacterAirResourceState* out){
    return character_air_resource_recover(input,rules.recovery_gate,flags5e0,cached87,rules.limit,rules.increment,out);
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
inline bool character_air_resource_consume(const CharacterAirResourceState& input,const AirMeterRules& rules,u8 cost_index,
                                            f32 rate5f4,CharacterAirResourceState* out){
    return cost_index<rules.costs.size()&&character_air_resource_consume(input,rules.costs[cost_index],rules.delay,rate5f4,out);
}
}
