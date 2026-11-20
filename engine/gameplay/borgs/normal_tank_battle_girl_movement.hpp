#pragma once
#include "engine/gameplay/character_movement_input.hpp"
#include "engine/gameplay/metered_air_dash_scalar.hpp"

namespace gf {
struct TankGirlDashProfile {
    u8 movement_type=0;
    AirMeterRules meter;
    f32 ground_clock=0,air_clock=0,air_speed44=0,air_planar_change4c=0,air_vertical_change50=0;
};
struct NormalTankGirlSource {
    u16 model=0;
    u8 variant=0;
    static constexpr TankGirlDashProfile tank{1,{AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{200,90,150,50}},
        10,60,15,-0.10f,-0.02f};
    static constexpr TankGirlDashProfile girl{0,{AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{150,50,450,50}},
        20,30,28,-0.05f,-0.02f};
    constexpr const TankGirlDashProfile* profile()const{
        return variant?nullptr:model==0x0c00?&tank:model==0x0300?&girl:nullptr;
    }
};
struct TankGirlDashRequest {
    u8 selector=0,direction58f=0;
    u16 remaining_directions5d8=0;
    bool consume_direction=false;
};
// Two scoped selector leaves; ground checks no positive fuel, air does.
// The caller owns all preceding movement/class admission.
inline bool normal_tank_girl_dash_request(NormalTankGirlSource source,bool airborne_leaf,u8 selector,u32 flags5e0,
                                         bool earlier_declined,const CharacterMovementInputState& in,
                                         const CharacterAirResourceState& resource,TankGirlDashRequest* out){
    const auto* p=source.profile();
    if(!out||!p||!earlier_declined||!character_movement_input_state_valid(in)||(in.commands5d4&~3u)||
       !metered_air_dash_detail::valid(p->meter,resource)||
       (airborne_leaf?(selector!=5&&selector!=6&&selector!=17):selector!=0))return false;
    TankGirlDashRequest next{selector,0,in.directions5d8,false};
    if(!(flags5e0&1u)&&(in.directions5d8&0xf0u)&&(!airborne_leaf||resource.value736>0))
        next={u8(airborne_leaf?15:12),u8((in.directions5d8>>4)&15u),u16(in.directions5d8&0xff0fu),true};
    *out=next;return true;
}

struct TankGirlGroundReleaseStep {
    f32 remaining568=0;
    bool open_recovery545=false;
};
// Ground release clock is gated by the authored lower control byte; caller
// owns the orientation-dependent held bit and the later contact/handoff.
inline bool normal_tank_girl_ground_release_step(NormalTankGirlSource source,f32 clock568,f32 rate1dc8,
                                                u8 lower_control1d0f,bool remembered_direction_held,
                                                TankGirlGroundReleaseStep* out){
    const auto* p=source.profile();
    if(!out||!p||!std::isfinite(clock568)||clock568<0||clock568>p->ground_clock||
       !metered_air_dash_detail::rate(rate1dc8))return false;
    const f32 remaining=lower_control1d0f?f32(clock568-rate1dc8):clock568;
    *out={remaining,lower_control1d0f&&(remaining<=0||!remembered_direction_held)};return true;
}

struct TankGirlActiveLatchStep {
    f32 remaining568=0;
    bool opens_release_and_landing=false;
};
// 6165C's ordinary phase-one policy-1 prefix. Opening the latch authorizes
// a later release or landing callback, not a timed transition to recovery.
inline bool normal_tank_girl_active_latch_step(NormalTankGirlSource source,f32 clock568,f32 rate5f4,
                                              TankGirlActiveLatchStep* out){
    const auto* p=source.profile();
    if(!out||!p||!std::isfinite(clock568)||clock568<0||clock568>p->air_clock||
       !metered_air_dash_detail::rate(rate5f4))return false;
    const f32 remaining=f32(clock568-rate5f4);
    *out={remaining,remaining<=0};return true;
}
inline bool normal_tank_girl_air_release_request(NormalTankGirlSource source,u8 ready588,u32 held5b4,u8* out){
    if(!out||!source.profile()||(held5b4&~0x000f0100u))return false;
    *out=ready588&&!(held5b4&0xf0000u)?17:16;return true;
}
inline bool normal_tank_girl_windup_step(NormalTankGirlSource source,const CharacterAirResourceState& in,
                                        f32 clock568,f32 rate1dc8,f32 rate5f4,MeteredAirDashWindupStep* out){
    const auto* p=source.profile();
    return p&&metered_air_dash_windup_step(p->meter,in,clock568,rate1dc8,rate5f4,out);
}
inline bool normal_tank_girl_active_debit(NormalTankGirlSource source,const CharacterAirResourceState& in,
                                         f32 rate5f4,u8 ready588,MeteredAirDashActiveDebit* out){
    const auto* p=source.profile();
    return p&&metered_air_dash_active_debit(p->meter,in,rate5f4,ready588,out);
}

struct TankGirlActionLanes {
    u8 upper=0,lower=0;
    bool writes_lower=false;
};
// Lane writes after the exact ordinary neutral B row is selected. This
// neither requests an attack nor claims an authored shot or hit.
inline bool normal_tank_girl_neutral_b_lanes(NormalTankGirlSource source,u8 old_lower,bool exact_neutral_row_selected,
                                            TankGirlActionLanes* out){
    if(!out||!source.profile()||!exact_neutral_row_selected||old_lower>17)return false;
    const bool replaces_lower=source.model==0x0300;
    *out={44,replaces_lower?u8(44):old_lower,replaces_lower};return true;
}
}
