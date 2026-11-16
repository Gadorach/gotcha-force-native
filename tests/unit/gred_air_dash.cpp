#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_air_dash.hpp"
#include "engine/gameplay/gred_movement_admission.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source air dash: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile motion_profile(){
    gf::CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.dimensions.fill(20);p.epsilon=.0001f;p.angle_unit=gf::bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};
    p.special_min_vertical=-35;p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;
    p.air_damping={.98f,.02f,.99f,.01f};p.ascent_pitch=8192;p.air_dash_operands=true;p.air_dash_windup_factor=.5f;p.air_dash_contact_delay=30;
    p.air={.8f,15,15,6,4,30,-.05f,0,30,0,-1,-.07f};return p;
}
gf::CharacterMovementInputProfile input_profile(){
    gf::CharacterMovementInputProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.held_mask=0xf0100;p.resource_policy=1;
    p.low_threshold=20;p.high_threshold=52;p.axis_window=2;p.other_window=3;p.direction_window=20;p.release_window=16;
    p.octants={2,10,8,9,1,5,4,6};p.resource_limit=10000;p.costs={150,100,450,50};p.recovery_increment=2000;
    for(auto& c:p.commands){c.kind=1;c.pulse=1;c.masks={1,0};}return p;
}
}
int main(){
    using namespace gf;auto p=motion_profile();const auto input=input_profile();
    check(character_movement_profile_valid(p)&&character_movement_input_profile_valid(input),"owned normal-G-Red policy1 fixtures");
    GRedAirDashState state;state.timer568=6;CharacterMovementMotionState motion;
    motion.position[1]=500;motion.speed44=8;motion.vertical48=16;motion.flags5e0=0x40;
    GRedAirDashPlan plan;CharacterAirResourceState resource;resource.value736=10000;
    check(gred_air_dash_plan(p,state,motion,&plan)&&plan.request&&plan.program==10&&!plan.move&&plan.motion.speed44==4&&
        plan.motion.vertical48==8&&plan.motion.vertical_change50==-.07f&&plan.cost_index==255,"windup entry pauses translation and halves old speeds, without an invented frame timer or entry spend");
    state=plan.state;motion=plan.motion;
    CharacterMovementCeiling ceiling;ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;ceiling.stage_top34=20000;ceiling.query_height668=80;
    for(unsigned tick=0;tick<6;++tick){
        check(gred_air_dash_plan(p,state,motion,&plan)&&plan.advance&&plan.turn_windup&&plan.move,"windup updates original motion/turn");
        CharacterMovementMotionState moved;
        check(character_movement_air_step(p,plan.motion,1,1,false,ceiling,&moved,&error),"shared unwrapped windup physics");
        if(plan.cost_after_move)check(character_air_resource_consume(resource,input.costs[plan.cost_index],0,1,&resource),"windup completion spends after movement");
        check(gred_air_dash_spent(plan,resource.value736,&state),"windup post-spend state");
        motion=moved;
    }
    check(state.selector==16&&!state.entered&&resource.value736==9550,"windup turn clock selects active and pays source cost450 exactly once");
    check(gred_air_dash_plan(p,state,motion,&plan)&&plan.request&&plan.program==11&&plan.motion.speed44==30&&plan.motion.vertical48==0&&
        plan.state.timer568==30&&plan.cost_index==3&&plan.steer_active,"active entry resets launch velocities and uses program11 plus source timer");
    check(character_air_resource_consume(resource,input.costs[3],0,1,&resource)&&gred_air_dash_spent(plan,resource.value736,&state),"active entry pays continuation cost50");
    motion=plan.motion;
    for(unsigned tick=0;tick<40;++tick){
        check(gred_air_dash_plan(p,state,motion,&plan)&&plan.advance&&plan.program==11,"active handler continues independently of VM completion");
        check(character_air_resource_consume(resource,input.costs[3],0,1,&resource)&&gred_air_dash_spent(plan,resource.value736,&state),"active meter consumption");
        motion=plan.motion;
    }
    check(state.selector==16&&state.stage==2&&state.timer568==0&&state.air_ready588==1&&state.contact_callback&&
        motion.planar_change4c==0,"policy1 timer enables release/contact without forcing dash termination");
    CharacterMovementInputState held;held.previous_held=8u<<16;GRedMovementAdmissionFields fields;
    fields.selector=16;fields.air_ready588=state.air_ready588;fields.flags5e0=0xc0;fields.prior_handlers_declined=true;
    GRedMovementAdmission admission;
    check(gred_movement_admit(input,held,resource,fields,&admission,&error)&&!admission.changed,"held direction may continue after minimum timer");
    held.previous_held=0;
    check(gred_movement_admit(input,held,resource,fields,&admission,&error)&&admission.changed&&admission.selector==17,"neutral direction after ready selects recovery through source predicate");
    state={};state.selector=16;resource.value736=25;
    check(gred_air_dash_plan(p,state,motion,&plan)&&character_air_resource_consume(resource,input.costs[3],0,1,&resource)&&
        gred_air_dash_spent(plan,resource.value736,&state)&&state.selector==17&&!state.entered&&!state.air_ready588&&!state.contact_callback,"early depletion selects recovery while preserving not-ready state and revoking contact callback");
    motion=plan.motion;
    check(gred_air_dash_plan(p,state,motion,&plan)&&plan.program==12&&plan.request&&plan.state.air_ready588&&plan.state.contact_callback&&
        plan.motion.vertical_change50==-1&&plan.motion.planar_change4c==-.05f,"recovery restores landing callback, selects original program12 and retains early-depletion planar change");
    GRedAirDashContactTail tail;
    check(gred_air_dash_contact_tail(17,true,false,false,&tail)&&!tail.landing&&!tail.falling,
        "above/leaving/steep-push recovery without VM completion continues program12, no landing or early falling");
    check(gred_air_dash_contact_tail(17,true,true,false,&tail)&&tail.falling&&!tail.landing,
        "617E4 lower completion after non-landing contact, including push, selects falling rather than replaying recovery");
    check(!gred_air_dash_contact_tail(17,true,true,true,&tail)&&tail.falling&&!tail.landing,
        "unreviewed simultaneous completion/landing refuses atomically");
    check(gred_air_dash_contact_tail(17,true,false,true,&tail)&&tail.landing&&!tail.falling,
        "real entered recovery touchdown remains a separate landing decision");
    check(!gred_air_dash_contact_tail(15,false,false,true,&tail)&&!gred_air_dash_contact_tail(8,true,false,false,&tail),
        "windup landing callback absence and unrelated relaunch selector cannot borrow recovery tail");
    auto legacy=p;legacy.air_dash_operands=false;
    check(!gred_air_dash_plan(legacy,state,motion,&plan),"old operand profiles cannot fabricate air-dash constants");
    auto malformed=state;malformed.entered=true;malformed.stage=0;
    check(!gred_air_dash_plan(p,malformed,motion,&plan),"uninitialized recovery stage cannot be advanced as a source callback");
    p.model=10;check(!gred_air_dash_plan(p,state,motion,&plan),"G-Red controller is not generalized to Sasuke");
    std::printf("source air-dash windup/active/recovery/policy1 resource planning: %u checks PASS; numeric candidates only, no actor/contact equivalence\n",checks);
}
