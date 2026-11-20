#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "engine/gameplay/borgs/normal_tank_battle_girl_movement.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Tank/Battle Girl: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    constexpr NormalTankGirlSource tank{0x0c00,0},girl{0x0300,0};
    static_assert(tank.profile()->movement_type==1&&girl.profile()->movement_type==0);
    static_assert(tank.profile()->meter.matches_operands(10000,0,2000,{200,90,150,50})&&
        girl.profile()->meter.matches_operands(10000,0,2000,{150,50,450,50}));
    for(auto source:{tank,girl}){
        const auto* p=source.profile();const bool is_tank=source.model==0x0c00;
        check(p->ground_clock==(is_tank?10.f:20.f)&&p->air_clock==(is_tank?60.f:30.f),
            "same selector paths use exact separate time seeds");
        CharacterMovementInputState in;in.directions5d8=0xa855;
        TankGirlDashRequest request;
        check(normal_tank_girl_dash_request(source,false,0,0,true,in,{},&request)&&request.selector==12&&
            request.consume_direction&&request.direction58f==5&&request.remaining_directions5d8==0xa805,
            "ground dash can admit without fuel and only consumes its nibble");
        check(normal_tank_girl_dash_request(source,true,6,0x40,true,in,{},&request)&&request.selector==6&&
            !request.consume_direction&&request.remaining_directions5d8==0xa855,"air requires fuel, refusal leaves flick");
        const CharacterAirResourceState fuel{200,0,2};
        check(normal_tank_girl_dash_request(source,true,6,0x40,true,in,fuel,&request)&&request.selector==15&&
            request.consume_direction&&fuel.value736==200,"air request takes no debit");
        check(!normal_tank_girl_dash_request(source,true,6,0x40,false,in,fuel,&request)&&request.selector==15,
            "earlier actor arbitration remains required");
        TankGirlGroundReleaseStep ground;
        check(normal_tank_girl_ground_release_step(source,1,1,0,false,&ground)&&ground.remaining568==1&&!ground.open_recovery545,
            "unserviced program gate neither ticks nor releases");
        check(normal_tank_girl_ground_release_step(source,2,1,3,false,&ground)&&ground.remaining568==1&&ground.open_recovery545,
            "remembered directional bucket release is separate from expiry");
        check(normal_tank_girl_ground_release_step(source,1,1,1,true,&ground)&&!ground.remaining568&&ground.open_recovery545,
            "eligible expiry opens ground recovery even while held");
        MeteredAirDashWindupStep windup;
        check(normal_tank_girl_windup_step(source,fuel,1,1,1,&windup)&&windup.selector==16&&
            windup.resource.value736==(is_tank?50:0),"different handoff costs, neither retests fuel");
        TankGirlActiveLatchStep latch;
        check(normal_tank_girl_active_latch_step(source,2,0.5f,&latch)&&latch.remaining568==1.5f&&!latch.opens_release_and_landing,
            "positive active clock leaves latch closed");
        MeteredAirDashActiveDebit debit;
        check(normal_tank_girl_active_debit(source,{49,0,2},1,0,&debit)&&debit.selector==17&&!debit.ready588,
            "depletion can recover before latch opens");
        check(normal_tank_girl_active_latch_step(source,1,1,&latch)&&latch.opens_release_and_landing,
            "expiry opens latch, not recovery by itself");
        u8 selector=255;
        check(normal_tank_girl_air_release_request(source,1,0x80000,&selector)&&selector==16&&
            normal_tank_girl_air_release_request(source,0,0,&selector)&&selector==16&&
            normal_tank_girl_air_release_request(source,1,0,&selector)&&selector==17,
            "fuelled held, early release and ready release are distinct boundaries");
        TankGirlActionLanes lanes;
        check(normal_tank_girl_neutral_b_lanes(source,2,true,&lanes)&&lanes.upper==44&&
            lanes.lower==(is_tank?2:44)&&lanes.writes_lower==!is_tank,"exact B lane ownership");
    }
    TankGirlActiveLatchStep latch;
    check(!normal_tank_girl_active_latch_step({0x0615,0},1,1,&latch)&&
        !normal_tank_girl_active_latch_step({0x0c00,1},1,1,&latch)&&
        !normal_tank_girl_active_latch_step(girl,60,1,&latch),"no cross-model or alternate active clock substitution");
    std::printf("normal Tank/Battle Girl shared dash gates and exact clocks: %u checks PASS\n",checks);
}
