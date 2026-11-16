#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_ground_dash.hpp"
#include "engine/gameplay/gred_vertical_jump.hpp"
#include "engine/gameplay/gred_movement_admission.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source dash jump: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile motion_profile(){
    gf::CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.dimensions.fill(20);p.epsilon=.0001f;p.angle_unit=gf::bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};p.special_min_vertical=-35;
    p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;p.ascent_pitch=8192;
    for(auto& d:p.ground_dash)d={25,-.5f,22,30};
    return p;
}
gf::CharacterMovementInputProfile input_profile(){
    gf::CharacterMovementInputProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.held_mask=0xf0100;p.resource_policy=1;
    p.low_threshold=20;p.high_threshold=52;p.axis_window=2;p.other_window=3;p.direction_window=20;p.release_window=16;
    p.octants={2,10,8,9,1,5,4,6};p.resource_limit=10000;
    for(auto& c:p.commands){c.kind=1;c.pulse=1;c.masks={512,0};}
    p.commands[4].pulse=16;p.commands[4].masks[0]=256;p.commands[5].flags=4;p.commands[5].masks[0]=256;return p;
}
}
int main(){
    using namespace gf;const auto p=motion_profile();const auto ip=input_profile();
    check(character_movement_profile_valid(p)&&character_movement_input_profile_valid(ip),"owned normal-G-Red fixtures");
    CharacterMovementInputState input,pressed;input.previous_held=8u<<16;
    check(character_movement_input_step(ip,input,{0,32,0x80100,0x20},&pressed,&error)&&
        (pressed.commands5d4&1)&&!(pressed.commands5d4&2),"grounded jump press produces buffered ground jump, not airborne relaunch");
    CharacterAirResourceState resource; //ground takeoff does not require a positive meter
    GRedMovementAdmissionFields fields;fields.flags5e0=0x20;fields.prior_handlers_declined=true;
    for(u8 selector:{u8(12),u8(13),u8(14)}){
        fields.selector=selector;GRedMovementAdmission admission;
        check(gred_movement_admit(ip,pressed,resource,fields,&admission,&error)&&admission.changed&&admission.selector==4&&
            admission.reset_direction57c&&!admission.consume_direction,"all reviewed ground-dash selectors admit buffered/held jump and reset57C");
        CharacterMovementMotionState motion;motion.speed44=25;motion.planar_change4c=-.5f;motion.flags5e0=0x20;motion.travel5ae=12345;
        GRedVerticalJumpPlan takeoff;
        check(gred_vertical_jump_plan(p,{4,0,false},motion,0,0x80100,&takeoff,true)&&takeoff.request&&takeoff.program==2&&!takeoff.air_move&&
            takeoff.motion.speed44==0&&takeoff.motion.planar_change4c==0&&takeoff.motion.vertical48==0&&takeoff.motion.vertical_change50==0&&
            takeoff.motion.travel5ae==12345&&takeoff.cost_index==255,"normal class12 enters existing takeoff without old dash motion, acceleration or launch cost");
    }
    CharacterMovementInputState held;
    check(character_movement_input_step(ip,pressed,{0,32,0x80100,0x20},&held,&error)&&!held.pressed5bc&&(held.commands5d4&1),"jump remains buffered after its press edge");
    fields.selector=13;GRedMovementAdmission admission;
    check(gred_movement_admit(ip,held,resource,fields,&admission,&error)&&admission.changed,"dash jump consumes source buffered intent, not a host-only pressed edge");
    fields.flags5e0|=2;
    check(gred_movement_admit(ip,held,resource,fields,&admission,&error)&&!admission.changed,"source jump lock still blocks interruption");
    fields.flags5e0=0x20;held.previous_held&=~0x100u;
    check(gred_movement_admit(ip,held,resource,fields,&admission,&error)&&!admission.changed,"buffered command without held jump cannot interrupt");
    held.previous_held|=0x100;held.commands5d4=0;
    check(gred_movement_admit(ip,held,resource,fields,&admission,&error)&&!admission.changed,"held jump without source command cannot interrupt");
    GRedGroundDashState recovery;recovery.selector=14;GRedGroundDashPlan refused;
    check(!gred_ground_dash_plan(p,recovery,25,-.5f,2,&refused),"unreviewed forced-continuation recovery is still refused when it executes");
    fields.selector=14;
    check(gred_movement_admit(ip,pressed,resource,fields,&admission,&error)&&admission.selector==4,"admitted jump selects takeoff before the old recovery callback, rather than bypassing its refusal");
    std::printf("source ground dash->takeoff admission/order/speed reset: %u checks PASS; normal G-Red numeric subset, no whole-frame equivalence\n",checks);
}
