#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_flame_dragon_movement.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Flame Dragon: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using S=NormalFlameDragonSource;
    using I=FlameDragonIntent;
    constexpr S source{0x0500,0};
    static_assert(S::movement_type==4&&S::resource_policy==1&&S::takeoff_state==56&&
        S::meter.matches_operands(10000,0,2000,{100,25,200,15}));
    CharacterMovementInputState input;
    CharacterAirResourceState resource{100,0,3};
    FlameDragonRequest request;
    auto run=[&](I intent,u8 selector,u32 flags,u8 latch,u8 expected){
        check(normal_flame_dragon_request(source,intent,selector,flags,latch,true,input,resource,&request)&&
            request.selector==expected,"scoped selector request");
    };
    input.previous_held=0x100;
    run(I::IdleJump,0,0,0,0);
    input.commands5d4=1;
    run(I::IdleJump,0,2,0,0);
    run(I::IdleJump,0,0,0,4);
    check(request.reset_direction57c&&resource.value736==100,"jump request does not spend meter");
    FlameDragonIdleTakeoff start;
    check(normal_flame_dragon_idle_takeoff(source,4,0,0,0xffff,&start)&&start.flags5e0==0xdfaf&&
        start.cached87==0&&start.phase544==3&&start.context547==2&&start.lower_mask==2&&start.program_group==0&&start.program==2,
        "original idle-derived lower state-56 prefix");
    check(!normal_flame_dragon_idle_takeoff(source,4,0,7,0,&start)&&start.phase544==3,"other predecessors aren't idle startup");
    u8 selector=255;
    check(normal_flame_dragon_takeoff_handoff(source,3,false,&selector)&&selector==4&&
        normal_flame_dragon_takeoff_handoff(source,3,true,&selector)&&selector==5,
        "lower program result gates the real ascent handoff");
    input.commands5d4=0;input.directions5d8=0xa828;
    run(I::GroundDash,0,0,0,0);
    check(!request.consume_direction&&request.remaining_directions5d8==0xa828,"missing ground row does not consume flick");
    run(I::AirDash,5,0x40,0,15);
    check(request.consume_direction&&request.direction58f==2&&request.remaining_directions5d8==0xa808&&resource.value736==100,
        "air request consumes only the flick nibble before any spend");
    run(I::AirDash,17,0x41,0,17);
    input.commands5d4=2;
    run(I::Relaunch,6,0x40,0,6);
    run(I::Relaunch,17,0x40,1,8);
    run(I::Relaunch,16,0x40,0,8);
    resource.value736=0;
    run(I::Relaunch,16,0x40,0,16);
    run(I::ReleaseAscent,5,0x40,1,6);
    run(I::ReleaseAirDash,16,0x40,1,17);
    check(!normal_flame_dragon_request(source,I::IdleJump,0,0,0,false,input,resource,&request)&&
        !normal_flame_dragon_request({0x0615,0},I::IdleJump,0,0,0,true,input,resource,&request)&&request.selector==17,
        "no override or G-Red inheritance");

    FlameDragonWindupStep wind;
    check(normal_flame_dragon_windup_step(source,{100,0,3},2,1,1,&wind)&&wind.selector==15&&wind.resource.value736==100,
        "positive clock does not debit yet");
    check(normal_flame_dragon_windup_step(source,{100,0,3},1,1,1,&wind)&&wind.selector==16&&!wind.resource.value736,
        "due handoff can spend the remaining meter and still enter active");
    FlameDragonActiveDebit active;
    check(normal_flame_dragon_active_debit(source,{12,0,3},1,1,&active)&&active.selector==17&&
        active.ready588==1&&!active.resource.value736,"depleted active visit recovers without landing or dropping readiness");
    check(normal_flame_dragon_active_debit(source,{25,0,3},1,0,&active)&&active.selector==16&&active.resource.value736==10,
        "positive active visit independently pays Dragon cost 15");
    check(normal_flame_dragon_charge(source,{125,0,3},AirMeterCharge::LiftEntry,1,&resource)&&resource.value736==25&&
        normal_flame_dragon_charge(source,resource,AirMeterCharge::LiftUpdate,1,&resource)&&!resource.value736,
        "distinct ascent entry and update costs");
    resource={8500,1,3};
    check(normal_flame_dragon_refill(source,resource,1,&resource)&&resource.value736==8500&&resource.delay73a==1&&
        resource.air_actions747==3,"airborne cache blocks refill even if a floor snap was seen");
    check(normal_flame_dragon_refill(source,resource,0,&resource)&&resource.value736==8500&&resource.delay73a==0&&
        resource.air_actions747==0,"grounded visit can tick delay and clear count without refill");
    check(normal_flame_dragon_refill(source,resource,0,&resource)&&resource.value736==10000,"following grounded visit refills and caps");
    std::printf("normal Flame Dragon bounded movement and shared meter: %u checks PASS\n",checks);
}
