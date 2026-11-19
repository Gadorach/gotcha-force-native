#pragma once
#include "engine/gameplay/borgs/normal_gred_movement_admission.hpp"
#include "engine/gameplay/character_movement_input.hpp"
#include "engine/gameplay/character_air_resource.hpp"
namespace gf {
struct GRedMovementAdmissionFields {
    u8 selector=0,air_ready588=0;u32 flags5e0=0;
    //Declared boundary, NOT an actor-admission token: source terrain/weapon/
    //auxiliary producers must actually have declined before applying this result.
    bool prior_handlers_declined=false;
};
struct GRedMovementAdmission {
    u8 selector=0,direction58f=0;u16 remaining_directions=0;
    bool changed=false,reset_direction57c=false,consume_direction=false,contact_tail_required=false;
};
//G-Red type0/policy1 movement predicates only. Clip completion, meter spending,
//physics, contact tails, attack admission and actor writes belong to their owners.
inline bool gred_movement_admit(const CharacterMovementInputProfile& p,const CharacterMovementInputState& input,
                                const CharacterAirResourceState& resource,const GRedMovementAdmissionFields& fields,
                                GRedMovementAdmission* out,std::string* error){
    auto fail=[&](){if(error)*error="G-Red movement admission: normal profile/declined prior handlers/source state required";return false;};
    if(!out||!character_movement_input_profile_valid(p)||p.model!=0x615||p.movement_type||p.resource_policy!=1||
       !character_movement_input_state_valid(input)||!fields.prior_handlers_declined||fields.air_ready588>1||
       (input.commands5d4&~3u)||resource.value736<0||resource.value736>p.resource_limit)return fail();
    using Rules=NormalGRedMovementAdmissionRules;
    const auto* rule=Rules::for_selector(fields.selector);
    if(!rule)return fail();
    GRedMovementAdmission result;result.selector=fields.selector;result.remaining_directions=input.directions5d8;
    auto select=[&](u8 selector){result.selector=selector;result.changed=true;return true;};
    auto jump=[&](){if(!(fields.flags5e0&2u)&&(input.commands5d4&1u)&&(input.previous_held&0x100u)){result.reset_direction57c=true;return select(4);}return false;};
    auto dash=[&](bool air){
        if((fields.flags5e0&1u)||!(input.directions5d8&0xf0u)||(air&&resource.value736<1))return false;
        result.direction58f=u8((input.directions5d8>>4)&15u);result.consume_direction=true;result.remaining_directions&=0xff0fu;
        return select(air?15:12);
    };
    auto relaunch=[&](bool needs_ready){
        return !(fields.flags5e0&2u)&&(input.commands5d4&2u)&&(!needs_ready||fields.air_ready588)&&resource.value736>0&&select(8);
    };
    auto release_flight=[&](){
        return (fields.flags5e0&0x40u)&&fields.air_ready588&&(resource.value736<1||!(input.previous_held&0x100u))&&select(6);
    };
    auto try_intent=[&](Rules::Intent intent){
        using Intent=Rules::Intent;
        switch(intent){
        case Intent::Jump:return jump();
        case Intent::GroundDash:return dash(false);
        case Intent::AirDash:return dash(true);
        case Intent::Walk:return !(fields.flags5e0&2u)&&(input.previous_held&0xf0000u)&&select(1);
        case Intent::Stop:return !(input.previous_held&0xf0000u)&&select(3);
        case Intent::ReleaseLift:return release_flight();
        case Intent::ReadyRelaunch:return relaunch(true);
        case Intent::Relaunch:return relaunch(false);
        case Intent::ReleaseDash:return !(input.previous_held&0xf0000u)&&fields.air_ready588&&select(17);
        case Intent::ContactTail:result.contact_tail_required=true;return true;
        case Intent::None:break;
        }
        return false;
    };
    for(auto intent:rule->order)if(intent==Rules::Intent::None||try_intent(intent))break;
    *out=result;if(error)error->clear();return true;
}
}
