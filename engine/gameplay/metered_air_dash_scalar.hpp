#pragma once
#include "engine/gameplay/character_air_resource.hpp"

namespace gf {
namespace metered_air_dash_detail {
inline bool valid(const AirMeterRules& rules,const CharacterAirResourceState& state){
    return state.value736>=0&&state.value736<=rules.limit&&state.delay73a>=0;
}
inline bool rate(f32 value){return std::isfinite(value)&&value>=0&&value<=16;}
}
struct MeteredAirDashWindupStep {
    CharacterAirResourceState resource;
    f32 remaining568=0;
    u8 selector=15;
};
// Shared 61488 handoff scalar branch on reviewed policy-1 paths. The
// caller owns the phase and two clocks; no live motion/pose is applied.
inline bool metered_air_dash_windup_step(const AirMeterRules& rules,const CharacterAirResourceState& in,
                                        f32 clock568,f32 rate1dc8,f32 rate5f4,MeteredAirDashWindupStep* out){
    using namespace metered_air_dash_detail;
    if(!out||!valid(rules,in)||!std::isfinite(clock568)||clock568<0||clock568>1e5f||!rate(rate1dc8)||!rate(rate5f4))return false;
    MeteredAirDashWindupStep next{in,f32(clock568-rate1dc8),15};
    if(next.remaining568<=0){
        if(!character_air_resource_consume(in,rules,u8(AirMeterCharge::DashHandoff),rate5f4,&next.resource))return false;
        next.selector=16; // no positive-meter retest after spending
    }
    *out=next;return true;
}

struct MeteredAirDashActiveDebit {
    CharacterAirResourceState resource;
    u8 selector=16,ready588=0;
};
// Shared 6170C/depletion scalar branch; readiness survived the selector
// write, and a zero debit result requests recovery rather than landing.
inline bool metered_air_dash_active_debit(const AirMeterRules& rules,const CharacterAirResourceState& in,
                                         f32 rate5f4,u8 ready588,MeteredAirDashActiveDebit* out){
    using namespace metered_air_dash_detail;
    if(!out||!valid(rules,in)||!rate(rate5f4))return false;
    MeteredAirDashActiveDebit next;
    if(!character_air_resource_consume(in,rules,u8(AirMeterCharge::DashActive),rate5f4,&next.resource))return false;
    next.ready588=ready588;
    if(!next.resource.value736)next.selector=17;
    *out=next;return true;
}
}
