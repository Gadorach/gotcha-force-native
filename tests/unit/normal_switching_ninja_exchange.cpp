#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_switching_ninja_exchange.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Switching Ninja exchange: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    SwitchingNinjaContact contact{9,0,20,12,true,{3,0,0x10,0,0,{11,20,30},3},{1,0,0,0x11,0,{-700,90,800},10}};
    SwitchingNinjaStagedExchange staged;
    check(normal_switching_ninja_stage_exchange(contact,&staged)&&
        staged.caster==SwitchingNinjaLeg{true,3,{-700,97,800}}&&staged.recipient==SwitchingNinjaLeg{true,1,{11,13,30}}&&
        staged.caster_flags1d9==0x14&&staged.recipient_flags1da==0x15,"compensate both heights at actual contact");
    check(contact.caster.position==SwitchingNinjaPosition{11,20,30}&&
        contact.recipient.position==SwitchingNinjaPosition{-700,90,800},"staging cannot move a current root");
    const auto good=staged;
    auto bad=contact;bad.caster_model=5;
    check(!normal_switching_ninja_stage_exchange(bad,&staged)&&staged==good,"self-teleport doesn't inherit an exchange");
    bad=contact;bad.recipient.slot=bad.caster.slot;
    check(!normal_switching_ninja_stage_exchange(bad,&staged)&&staged==good,"two distinct observed slots required");
    bad=contact;bad.observed_resolved_contact=false;
    check(!normal_switching_ninja_stage_exchange(bad,&staged)&&staged==good,"no swap from a target hint");
    bad=contact;bad.recipient.receiver_mask59c=0x10;
    check(normal_switching_ninja_stage_exchange(bad,&staged)&&!staged.caster.queued&&!staged.recipient.queued,
        "masked recipient leaves both queues empty");
    bad=contact;bad.caster.flags1d9|=4;
    check(normal_switching_ninja_stage_exchange(bad,&staged)&&!staged.caster.queued&&staged.caster_flags1d9==0x14&&
        staged.recipient_flags1da==0x11,"cannot overwrite a pending leg");

    SwitchingNinjaTransferOwner caster{3,0,155,0x40,true,true},recipient{1,1,126,0x40,true,true};
    SwitchingNinjaTransferArm caster_arm,recipient_arm;
    check(normal_switching_ninja_arm_leg(good.recipient,recipient,&recipient_arm)&&!recipient_arm.request_selector35,
        "busy recipient skips its own arm");
    recipient.existing_mode7c9=0;recipient.hp_after_hit=0;
    check(normal_switching_ninja_arm_leg(good.recipient,recipient,&recipient_arm)&&!recipient_arm.request_selector35,
        "post-hit defeated recipient cannot arm independently");
    check(normal_switching_ninja_arm_leg(good.caster,caster,&caster_arm)&&caster_arm.request_selector35&&
        caster_arm.state==SwitchingNinjaTransferState{3,2,0,0,0x1040,0xffffu,{-700,97,800}},
        "caster advances without waiting for recipient acceptance");
    check(!normal_switching_ninja_arm_leg(good.recipient,caster,&caster_arm)&&caster_arm.state.slot==3,
        "a queued leg cannot be applied to the other slot");

    SwitchingNinjaTransferStep tick;
    check(normal_switching_ninja_transfer_step(caster_arm.state,&tick)&&tick.state.phase7ca==1&&tick.state.clock7cb==2&&
        !tick.place_this_slot&&!tick.finish_this_slot,"entry seeds local transfer clock");
    check(normal_switching_ninja_transfer_step(tick.state,&tick)&&tick.state.clock7cb==1&&!tick.place_this_slot,
        "first countdown tick doesn't move");
    check(normal_switching_ninja_transfer_step(tick.state,&tick)&&tick.state.phase7ca==2&&tick.state.clock7cb==2&&tick.place_this_slot,
        "second tick requests this slot's own placement");
    SwitchingNinjaPlacement placement;
    check(!normal_switching_ninja_place_leg(tick,{-500,500,-600,600,false},&placement)&&!placement.terrain_query_due,
        "unknown stage bounds do not invent a clamp");
    check(normal_switching_ninja_place_leg(tick,{-500,500,-600,600,true},&placement)&&placement.slot==3&&
        placement.position==SwitchingNinjaPosition{-500,97,600}&&placement.queued7bc==placement.position&&
        placement.previous2c==placement.position&&placement.older5e8==placement.position&&placement.terrain_query_due,
        "clamp X/Z in the queue, copy all three histories, leave terrain pending");
    check(normal_switching_ninja_transfer_step(tick.state,&tick)&&!tick.finish_this_slot&&tick.state.clock7cb==1,
        "finish waits for its later local callback");
    check(normal_switching_ninja_transfer_step(tick.state,&tick)&&tick.finish_this_slot&&!tick.place_this_slot&&
        tick.state.mode7c9==0&&tick.state.phase7ca==3&&tick.state.collision_mask272==0&&tick.state.flags5e0==0x40,
        "finish clears current transfer mask rather than inventing saved-mask restore");
    check(!normal_switching_ninja_transfer_step(tick.state,&tick)&&tick.finish_this_slot,"ordinary completion cannot run twice");
    std::printf("normal Switching Ninja staged exchange and independent transfer: %u checks PASS\n",checks);
}
