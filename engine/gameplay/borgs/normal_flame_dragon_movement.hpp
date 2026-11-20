#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_input.hpp"
#include "engine/gameplay/metered_air_dash_scalar.hpp"

namespace gf {
struct NormalFlameDragonSource {
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return model==0x0500&&!variant;}
    static constexpr u8 movement_type=4,resource_policy=1,takeoff_state=56;
    static constexpr AirMeterRules meter{AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{100,25,200,15}};
    static constexpr bool valid(const CharacterAirResourceState& s){return s.value736>=0&&s.value736<=meter.limit&&s.delay73a>=0;}
};
enum class FlameDragonIntent:u8 {IdleJump,GroundDash,AirDash,Relaunch,ReleaseAscent,ReleaseAirDash};
struct FlameDragonRequest {
    u8 selector=0,direction58f=0;
    u16 remaining_directions5d8=0;
    bool reset_direction57c=false,consume_direction=false;
};
// Explicitly addressed leaves, after earlier handlers have declined. A
// request doesn't spend meter, run a lower state, or evaluate floor contact.
inline bool normal_flame_dragon_request(NormalFlameDragonSource source,FlameDragonIntent intent,u8 selector,
                                        u32 flags5e0,u8 ready588,bool earlier_declined,const CharacterMovementInputState& in,
                                        const CharacterAirResourceState& resource,FlameDragonRequest* out){
    if(!out||!source.reviewed()||!earlier_declined||!NormalFlameDragonSource::valid(resource)||
       !character_movement_input_state_valid(in)||(in.commands5d4&~3u))return false;
    FlameDragonRequest next{selector,0,in.directions5d8};
    switch(intent){
    case FlameDragonIntent::IdleJump:
        if(selector!=0)return false;
        if(!(flags5e0&2u)&&(in.commands5d4&1u)&&(in.previous_held&0x100u)){
            next.selector=4;next.reset_direction57c=true;
        }
        break;
    case FlameDragonIntent::GroundDash:
        if(selector!=0)return false;
        break; // lower 12 is absent, so the success-only pulse consumption never occurs
    case FlameDragonIntent::AirDash:
        if(selector!=5&&selector!=6&&selector!=17)return false;
        if(!(flags5e0&1u)&&(in.directions5d8&0xf0u)&&resource.value736>0){
            next.selector=15;next.consume_direction=true;next.direction58f=u8((in.directions5d8>>4)&15u);
            next.remaining_directions5d8&=0xff0fu;
        }
        break;
    case FlameDragonIntent::Relaunch:
        if(selector!=6&&selector!=16&&selector!=17)return false;
        if(!(flags5e0&2u)&&(in.commands5d4&2u)&&(selector==16||ready588)&&resource.value736>0)next.selector=8;
        break;
    case FlameDragonIntent::ReleaseAscent:
        if(selector!=5)return false;
        if((flags5e0&0x40u)&&ready588&&(!resource.value736||!(in.previous_held&0x100u)))next.selector=6;
        break;
    case FlameDragonIntent::ReleaseAirDash:
        if(selector!=16)return false;
        if(ready588&&!(in.previous_held&0xf0000u))next.selector=17;
        break;
    default:return false;
    }
    *out=next;return true;
}

struct FlameDragonIdleTakeoff {
    u32 flags5e0=0;
    u8 cached87=0,phase544=0,context547=0,lower_mask=0,program_group=0,program=0;
};
// Only the idle-derived state-56 lower entry, not the other predecessor
// branches. Its local program requires a real VM; it doesn't imply ascent.
inline bool normal_flame_dragon_idle_takeoff(NormalFlameDragonSource source,u8 selector,u8 phase,u8 previous_selector,
                                            u32 flags5e0,FlameDragonIdleTakeoff* out){
    if(!out||!source.reviewed()||selector!=4||phase||previous_selector)return false;
    *out={flags5e0&~0x2050u,0,3,2,2,0,2};return true;
}
inline bool normal_flame_dragon_takeoff_handoff(NormalFlameDragonSource source,u8 phase,bool lower_program_done,u8* out){
    if(!out||!source.reviewed()||phase!=3)return false;
    *out=lower_program_done?5:4;return true;
}

inline bool normal_flame_dragon_refill(NormalFlameDragonSource source,const CharacterAirResourceState& in,u8 cached87,
                                      CharacterAirResourceState* out){
    return source.reviewed()&&cached87<=1&&NormalFlameDragonSource::valid(in)&&
        character_air_resource_recover(in,NormalFlameDragonSource::meter,0,cached87,out);
}

inline bool normal_flame_dragon_charge(NormalFlameDragonSource source,const CharacterAirResourceState& in,AirMeterCharge charge,
                                      f32 rate5f4,CharacterAirResourceState* out){
    return source.reviewed()&&NormalFlameDragonSource::valid(in)&&std::isfinite(rate5f4)&&rate5f4>=0&&rate5f4<=16&&
        character_air_resource_consume(in,NormalFlameDragonSource::meter,u8(charge),rate5f4,out);
}

using FlameDragonWindupStep=MeteredAirDashWindupStep;
// The reviewed handoff subtracts the movement clock, then pays the shared
// cost if due. It does not retest positive meter after the debit.
inline bool normal_flame_dragon_windup_step(NormalFlameDragonSource source,const CharacterAirResourceState& in,
                                           f32 clock568,f32 rate1dc8,f32 rate5f4,FlameDragonWindupStep* out){
    return source.reviewed()&&metered_air_dash_windup_step(NormalFlameDragonSource::meter,in,clock568,rate1dc8,rate5f4,out);
}

// Ordinary policy-1 active visit after its motion/contact work. The caller
// supplies the current readiness; zero meter selects recovery, not landing.
using FlameDragonActiveDebit=MeteredAirDashActiveDebit;
inline bool normal_flame_dragon_active_debit(NormalFlameDragonSource source,const CharacterAirResourceState& in,
                                            f32 rate5f4,u8 ready588,FlameDragonActiveDebit* out){
    return source.reviewed()&&metered_air_dash_active_debit(NormalFlameDragonSource::meter,in,rate5f4,ready588,out);
}
}
