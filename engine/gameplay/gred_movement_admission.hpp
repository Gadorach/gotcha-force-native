#pragma once
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
    switch(fields.selector){case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 12:case 13:case 14:case 15:case 16:case 17:break;default:return fail();}
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
    auto walk=[&](){return !(fields.flags5e0&2u)&&(input.previous_held&0xf0000u)&&select(1);};
    switch(fields.selector){
    case 0:case 3:if(!jump()&&!dash(false))walk();break;
    case 1:case 2:if(!jump()&&!dash(false)&&!(input.previous_held&0xf0000u))select(3);break;
    case 4:case 15:break; //source animation/motion handlers, not input, own continuation
    case 5:if(!release_flight())dash(true);break;
    case 6:case 17:if(!relaunch(true)&&!dash(true))result.contact_tail_required=true;break;
    case 7:if(!jump())dash(false);break;
    case 8:if(!release_flight()&&!dash(true))result.contact_tail_required=true;break;
    case 12:case 13:case 14:jump();break;
    case 16:
        if(!relaunch(false)){
            if(!(input.previous_held&0xf0000u)&&fields.air_ready588)select(17);
            else result.contact_tail_required=true;
        }
        break;
    }
    *out=result;if(error)error->clear();return true;
}
}
