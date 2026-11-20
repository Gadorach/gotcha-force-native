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

// Count-based movement uses the signed source byte at 747, not meter 736
// or recovery delay 73A. Only bounded, nonnegative in-play counts are modeled.
struct AirPointRules {
    u8 limit=0;
    bool seed_on_dash_windup=false;
    constexpr bool accepts(u8 used)const{return limit&&limit<=127&&used<=limit;}
    constexpr bool has_point(u8 used)const{return accepts(used)&&used<limit;}
};
enum class AirPointEntry:u8 {Ascent,Relaunch,DashWindup};

// Compute a candidate for an explicitly entered lower state, not a request.
// The caller still owns movement/pose acceptance and publication of 747.
inline bool character_air_point_entry(const AirPointRules& rules,u8 used,AirPointEntry entry,u8* out){
    if(!out||!rules.accepts(used))return false;
    switch(entry){
    case AirPointEntry::Ascent:case AirPointEntry::Relaunch:
        if(!rules.has_point(used))return false;
        *out=u8(used+1u);return true;
    case AirPointEntry::DashWindup:
        *out=(!used&&rules.seed_on_dash_windup)?1:used;return true;
    }
    return false;
}

struct AirPointGaugeReadout {
    bool draw_ready=false;
    u8 remaining=0; // only meaningful when draw-ready
    bool operator==(const AirPointGaugeReadout&)const=default;
};

// Count-gauge producer's immediate readout, not cached animation/fade state,
// ownership lookup, view gating, or a draw call.
inline bool character_air_point_gauge(const AirPointRules& rules,u8 used,bool actor_suppressed,AirPointGaugeReadout* out){
    if(!out||!rules.accepts(used))return false;
    AirPointGaugeReadout next;
    if(used&&!actor_suppressed)next={true,u8(rules.limit-used)};
    *out=next;return true;
}

struct AirMeterGaugeReadout {
    bool draw_ready=false;
    f32 sample=0;
    bool operator==(const AirMeterGaugeReadout&)const=default;
};

// Source meter-gauge sample, not opacity, a display lifetime, or a draw call.
inline bool character_air_meter_gauge(s16 meter,s16 capacity,bool actor_suppressed,AirMeterGaugeReadout* out){
    if(!out||capacity<=0||meter<0||meter>capacity)return false;
    AirMeterGaugeReadout next;
    if(!actor_suppressed){
        const f32 ratio=f32(meter)/f32(capacity);
        next.sample=1.f-ratio;next.draw_ready=next.sample!=0.f;
    }
    *out=next;return true;
}

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
