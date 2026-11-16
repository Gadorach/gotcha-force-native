#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_relaunch.hpp"
#include "engine/gameplay/gred_vertical_jump.hpp"
#include "engine/gameplay/gred_movement_admission.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source relaunch: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile profile(){
    gf::CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.dimensions.fill(20);p.epsilon=.0001f;p.angle_unit=gf::bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};p.special_min_vertical=-35;
    p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;p.ascent_pitch=8192;p.air_ready_delay=20;
    p.air_damping={.98f,.02f,.99f,.01f};p.air={.8f,15,15,6,4,30,-.05f,0,30,0,-1,-.07f};return p;
}
gf::CharacterMovementInputProfile inputs(){
    gf::CharacterMovementInputProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.held_mask=0xf0100;p.resource_policy=1;
    p.low_threshold=20;p.high_threshold=52;p.axis_window=2;p.other_window=3;p.direction_window=20;p.release_window=16;
    p.octants={2,10,8,9,1,5,4,6};p.resource_limit=10000;p.costs={150,100,450,50};
    for(auto& c:p.commands){c.kind=1;c.pulse=1;c.masks={1,0};}return p;
}
}
int main(){
    using namespace gf;auto p=profile();const auto ip=inputs();GRedRelaunchPlan plan;
    check(character_movement_profile_valid(p)&&character_movement_input_profile_valid(ip),"owned normal G-Red fixtures");
    CharacterMovementInputState input;input.previous_held=0x100;input.commands5d4=2;
    CharacterAirResourceState resource;resource.value736=1000;resource.air_actions747=255;
    GRedMovementAdmissionFields fields;fields.selector=6;fields.flags5e0=0x40;fields.prior_handlers_declined=true;GRedMovementAdmission admission;
    check(gred_movement_admit(ip,input,resource,fields,&admission,&error)&&!admission.changed,"falling relaunch waits for source readiness, not an invented double-jump key");
    fields.air_ready588=1;
    check(gred_movement_admit(ip,input,resource,fields,&admission,&error)&&admission.selector==8&&admission.changed,"fresh buffered airborne command plus resource admits8 without a fixed jump-count limit");
    fields.selector=16;fields.air_ready588=0;
    check(gred_movement_admit(ip,input,resource,fields,&admission,&error)&&admission.selector==8,"active dash relaunch does not require minimum dash timer readiness");
    resource.value736=0;
    check(gred_movement_admit(ip,input,resource,fields,&admission,&error)&&!admission.changed,"empty resource cannot admit relaunch");
    CharacterMovementMotionState motion;motion.position[1]=500;motion.vertical48=-20;
    check(gred_relaunch_plan(p,{},motion,&plan)&&plan.request&&plan.program==14&&plan.cost_index==0&&plan.motion.vertical48==-5,"entry adds15 to descending velocity instead of replacing it");
    motion.vertical48=18;
    check(gred_relaunch_plan(p,{},motion,&plan)&&plan.motion.vertical48==33,"rising entry preserves old velocity in its first displacement");
    auto accelerating=plan.motion;check(gred_jump_acceleration(p,false,&accelerating),"shared neutral acceleration");
    CharacterMovementCeiling ceiling;ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;ceiling.stage_top34=20000;ceiling.query_height668=80;
    CharacterMovementMotionState moved;
    check(character_movement_air_step(p,accelerating,1,1,true,ceiling,&moved,&error)&&moved.position[1]==533&&moved.vertical48==18,"source wrapper clamps velocity after old/additive velocity displacement");
    GRedRelaunchState state;
    check(gred_relaunch_lower_finished(plan,true,&state)&&state.lower_stage==1,"request alone is not lower VM completion");
    GRedRelaunchUpperPlan upper;
    check(gred_relaunch_upper_plan(state,&upper)&&upper.request&&upper.program==14&&upper.next_stage==1,"upper windup requests original program14");
    state.upper_stage=upper.next_stage;
    check(gred_relaunch_plan(p,state,moved,&plan)&&!plan.request&&plan.program==14&&plan.cost_index==1,"continuation updates windup and pays active cost once per lower callback");
    check(gred_relaunch_lower_finished(plan,true,&state)&&state.lower_stage==2&&
        gred_relaunch_upper_plan(state,&upper)&&!upper.request&&upper.program==14&&upper.next_stage==2,"upper still updates14 while observing new lower completion latch");
    state.upper_stage=upper.next_stage;
    check(gred_relaunch_plan(p,state,moved,&plan)&&plan.request&&plan.program==15&&
        gred_relaunch_lower_finished(plan,false,&state)&&state.lower_stage==3&&
        gred_relaunch_upper_plan(state,&upper)&&upper.request&&upper.program==15&&upper.next_stage==3,"next callback requests15 on both lanes, not a guessed timer");
    state.upper_stage=upper.next_stage;
    check(gred_relaunch_plan(p,state,moved,&plan)&&!plan.request&&plan.program==15,"active relaunch keeps original ascent program");
    GRedVerticalJumpState fall{6,1,false};fall.falling_after_relaunch=true;fall.falling_coarse57d=3;GRedVerticalJumpPlan falling;
    check(gred_vertical_jump_plan(p,fall,moved,500,0,&falling,true)&&falling.program==24,"relaunch release uses reviewed retained-coarse variant, not ordinary apex or air-dash fall");
    check(!gred_relaunch_plan(p,{0,1},motion,&plan),"upper relaunch stage cannot precede qualified lower entry");
    p.model=10;check(!gred_relaunch_plan(p,{},motion,&plan),"G-Red relaunch is not generalized to Sasuke");
    std::printf("source relaunch admission/additive velocity/lower-upper handoff: %u checks PASS; numeric candidates, no whole-frame equivalence\n",checks);
}
