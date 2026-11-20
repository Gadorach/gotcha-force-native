#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_input.hpp"

namespace gf {
// Scalar subset verified on the exact normal Blue Striker and Death ICBM
// game controllers. This type does not bind other type-3 models or actors.
struct Type3CruiseBoostRules {
    static constexpr u8 movement_type=3,resource_policy=1;
    static constexpr u8 cruise=27,boost=28,boost_debit_index=3;
    static constexpr f32 exit_clock=30.f;
    static constexpr AirMeterRules meter{AirMeterRecoveryGate::Behavior80Clear,10000,16,50,{0,0,0,50}};
    static constexpr bool valid_resource(const CharacterAirResourceState& state){
        return state.value736>=0&&state.value736<=meter.limit&&state.delay73a>=0&&state.delay73a<=meter.delay;
    }
    static bool valid_rate(f32 rate){return std::isfinite(rate)&&rate>=0&&rate<=16;}
};

// Resource pass uses the flags before current selector/handler writes.
// Type 3 has no cached-ground or hover-height condition on this branch.
inline bool type3_preselect_refill(const CharacterAirResourceState& in,u32 prior_flags5e0,CharacterAirResourceState* out){
    using R=Type3CruiseBoostRules;
    return R::valid_resource(in)&&character_air_resource_recover(in,R::meter,prior_flags5e0,0,out);
}

struct Type3BoostRequestFields {
    u8 selector=0,exit_latch588=0;
    u32 flags5e0=0;
    bool prior_handlers_declined=false;
};

// Only the conditioned cruise/boost leaves. No selector/latch write or handler
// work occurs here. The caller owns air-pulse production and earlier checks.
inline bool type3_boost_request(const CharacterMovementInputState& in,const Type3BoostRequestFields& fields,
                                 const CharacterAirResourceState& after_refill,u8* out){
    using R=Type3CruiseBoostRules;
    if(!out||!fields.prior_handlers_declined||!character_movement_input_state_valid(in)||(in.commands5d4&~3u)||
       !R::valid_resource(after_refill))return false;
    switch(fields.selector){
    case R::cruise:
        *out=!(fields.flags5e0&2u)&&(in.commands5d4&2u)&&after_refill.value736>0?R::boost:R::cruise;
        return true;
    case R::boost:
        *out=fields.exit_latch588&&(!after_refill.value736||!(in.previous_held&0x100u))?R::cruise:R::boost;
        return true;
    default:return false;
    }
}

struct Type3BoostDebit {
    CharacterAirResourceState resource;
    u32 flags5e0=0;
};

// Candidate prefix only for an already accepted lower boost-handler call.
// Zero meter does not re-arm the delay, but the boost bit is still set.
inline bool type3_boost_debit(const CharacterAirResourceState& in,u32 flags5e0,f32 rate5f4,Type3BoostDebit* out){
    using R=Type3CruiseBoostRules;
    if(!out||!R::valid_resource(in)||!R::valid_rate(rate5f4))return false;
    Type3BoostDebit next;
    if(!character_air_resource_consume(in,R::meter,R::boost_debit_index,rate5f4,&next.resource))return false;
    next.flags5e0=flags5e0|0x80u;
    *out=next;return true;
}

struct Type3CruiseEntry {
    u32 flags5e0=0;
    u8 cached87=0;
};

// Ordinary phase-zero cruise entry prefix only, after the selector is accepted.
constexpr Type3CruiseEntry type3_cruise_entry(u32 prior_flags5e0){
    return {(prior_flags5e0&~0x2080u)|0x40u,1};
}

struct Type3ExitClockStep {
    f32 remaining568=0;
    bool opens_exit_latch=false;
};

// Phase-one clock fragment; opening the latch isn't a cruise request.
// The other phase writes (including the vertical-motion change) are not applied.
inline bool type3_exit_clock(f32 before568,f32 rate5f4,Type3ExitClockStep* out){
    using R=Type3CruiseBoostRules;
    if(!out||!R::valid_rate(rate5f4)||!std::isfinite(before568)||before568<=0||before568>R::exit_clock)return false;
    const f32 remaining=before568-rate5f4;
    *out={remaining,remaining<=0.f};return true;
}
}
