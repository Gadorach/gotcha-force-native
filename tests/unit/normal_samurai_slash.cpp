#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_samurai_slash.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Samurai X: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using P=SamuraiSlashPosture;
    constexpr NormalSamuraiSlashSource demon{0x0701,0,2},ordinary{0x0700,0,3};
    SamuraiSlashRequest action;
    check(normal_samurai_slash_request(demon,P::Neutral,0x80,0,true,&action)&&action.requested&&action.selector==44&&
        action.class_state580==2&&!action.substate581,"neutral X chooses the exact stockless class row");
    check(normal_samurai_slash_request(ordinary,P::OrdinaryAirborne,0x80,0,true,&action)&&action.requested&&
        action.selector==45&&action.substate581==3,"audited airborne posture has its separate row");
    check(normal_samurai_slash_request(demon,P::Neutral,0x80,4,true,&action)&&!action.requested,"inhibited X does not stage");
    check(!normal_samurai_slash_request({0x0701,1,2},P::Neutral,0x80,0,true,&action)&&!action.requested,"no alternate variant substitute");
    SamuraiSlashProgram program;
    check(normal_samurai_slash_start_program(demon,P::OrdinaryAirborne,&program)&&program.requested&&program.slot==0&&
        normal_samurai_slash_start_program(ordinary,P::OrdinaryAirborne,&program)&&program.slot==3&&program.group==4&&
        program.mask==15,"same airborne action row doesn't select same local program");
    SamuraiSlashActiveEntry entry;
    check(normal_samurai_slash_active_entry(demon,false,&entry)&&!entry.program.requested,
        "startup alone doesn't admit the active phase");
    check(normal_samurai_slash_active_entry(demon,true,&entry)&&entry.program.requested&&entry.program.slot==1&&
        entry.counters==SamuraiSlashCounters{0,30,8},"exact Demon neutral active operands");
    SamuraiSlashCounters next,same;
    check(normal_samurai_slash_button_step(demon,entry.counters,0x80000000,&next)&&next==entry.counters,
        "other edge bits aren't part of this branch");
    check(normal_samurai_slash_button_step(demon,entry.counters,0x100,&next)&&next==SamuraiSlashCounters{8,22,7}&&
        normal_samurai_slash_button_step(demon,entry.counters,0xf00,&same)&&same==next,
        "one or many fresh bits transfer once without a random branch");
    check(normal_samurai_slash_button_step(demon,{5,1,2},0x400,&next)&&next==SamuraiSlashCounters{6,0,2},
        "reserve caps transfer and contribution has its exact lower bound");
    SamuraiSlashProgress progress;
    check(normal_samurai_slash_progress(demon,9,0,&progress)&&progress.phase540==3&&progress.active548==9&&
        !progress.program.requested,"no lane event, no clock subtraction");
    check(normal_samurai_slash_progress(demon,10,1,&progress)&&progress.active548==0&&progress.phase540==3&&
        !progress.program.requested,"zero is not strictly negative");
    check(normal_samurai_slash_progress(demon,9,255,&progress)&&progress.active548==-1&&progress.phase540==4&&
        progress.program.requested&&progress.program.slot==2,"nonzero lane event and negative clock queue exact next program");

    SamuraiSlashShapeRequest shape;
    check(normal_samurai_slash_shape(demon,1,2,true,&shape)&&!shape.requested,"wrong lane cannot produce hit group");
    check(normal_samurai_slash_shape(demon,1,15,true,&shape)&&shape.requested&&shape.carrier_slot==2&&shape.credited_slot==2&&
        shape.shape_group==7&&shape.hit_row==19&&shape.descriptor_count==3&&shape.descriptors==std::array<u8,3>{7,13,17}&&
        !shape.start&&shape.end==-2,"first Demon actor-owned group has three exact spans");
    check(normal_samurai_slash_shape(ordinary,1,15,true,&shape)&&shape.carrier_slot==3&&shape.descriptor_count==1&&
        shape.start==9&&shape.end==-2,"Normal geometry uses different operands despite same group and owner semantics");
    check(normal_samurai_slash_shape(ordinary,2,1,true,&shape)&&shape.shape_group==8&&shape.hit_row==20&&shape.descriptor_count==2,
        "following section binds its own group, not a presentation event");
    check(!normal_samurai_slash_shape(ordinary,0,1,true,&shape)&&shape.shape_group==8,"startup without group request cannot borrow one");
    SamuraiSlashVisualRequest visual;
    check(normal_samurai_slash_visual(demon,15,true,&visual)&&visual.requested&&visual.keys==std::array<u8,2>{19,18}&&
        normal_samurai_slash_visual(ordinary,15,true,&visual)&&visual.requested&&visual.keys==std::array<u8,2>{6,5}&&
        shape.shape_group==8,"separate key pairs cannot change active geometry");
    std::printf("normal Samurai neutral X, exact counters and actor-owned shape proposals: %u checks PASS\n",checks);
}
