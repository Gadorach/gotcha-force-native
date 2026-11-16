#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_vertical_jump.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"vertical source jump: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile profile(){
    gf::CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.dimensions.fill(20);p.epsilon=.0001f;p.angle_unit=gf::bits_to_f32(0x38c90fdbu);
    p.limits={8,18,-30};p.special_min_vertical=-35;p.air[0]=.8f;p.air[1]=15;p.air[10]=-1;return p;
}
}
int main(){
    using namespace gf;const auto p=profile();check(character_movement_profile_valid(p),"owned normal-G-Red numeric fixture");
    CharacterMovementMotionState motion;motion.position={-200,0,-200};GRedVerticalJumpState state{4,0,false};GRedVerticalJumpPlan plan;
    check(gred_vertical_jump_plan(p,state,motion,1000,0x100,&plan)&&plan.request&&plan.program==2&&!plan.air_move&&plan.state.cached87==0,"takeoff requests original program without launching early");
    check(gred_vertical_jump_finish(plan,true,false,&state)&&state.selector==4,"request completion is not takeoff update completion");
    check(gred_vertical_jump_plan(p,state,motion,1000,0x100,&plan)&&plan.advance&&
        gred_vertical_jump_finish(plan,true,false,&state)&&state.selector==5&&!state.entered,"lower VM completion selects initial ascent");
    check(gred_vertical_jump_plan(p,state,motion,1000,0x100,&plan)&&plan.program==4&&plan.cost_index==0&&
        plan.motion.vertical48==15&&plan.motion.vertical_change50==.8f&&plan.clamp_wrapper&&plan.state.cached87==1,"neutral source ascent retains cos(0) acceleration, entry cost and source clamp");
    CharacterAirResourceState resource;resource.value736=1000;
    check(character_air_resource_consume(resource,150,0,1,&resource)&&resource.value736==850,"entry spending uses shared recovery/consume kernel");
    motion=plan.motion;state=plan.state;
    check(gred_vertical_jump_plan(p,state,motion,850,0x100,&plan)&&plan.advance&&plan.cost_index==1&&plan.state.selector==5,"held ascent consumes continuation cost, not repeated entry cost");
    check(gred_vertical_jump_plan(p,state,motion,0,0x100,&plan)&&plan.state.selector==6&&plan.program==6&&plan.motion.vertical_change50==-1,"resource depletion selects falling even while held");
    check(gred_vertical_jump_plan(p,state,motion,850,0,&plan)&&plan.state.selector==6&&plan.request&&!plan.clamp_wrapper,"release selects original apex/falling program");
    using namespace stage_ground_fixture;
    auto data=bytes({triangle(0)}),empty_data=bytes({});StihGrid floor,empty;
    check(floor.load({data.data(),data.size()},&error)&&empty.load({empty_data.data(),empty_data.size()},&error),"owned flat static grids");
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=parameters();
    ground.actor.fields_declared=true;ground.query_height668=80;ground.step674=30;
    state=plan.state;motion=plan.motion;motion.position[1]=50;motion.flags5e0=0x40;
    bool landed=false;float maximum=50;unsigned falling_updates=0;
    for(unsigned tick=0;tick<100&&!landed;++tick){
        check(gred_vertical_jump_plan(p,state,motion,850,0,&plan),"bounded falling callback plan");
        const auto before=plan.motion;ground.actor.current24=before.position[1];ground.actor.previous30=before.position[1];
        CharacterMovementMotionState proposed,accepted;bool hit=false;
        CharacterMovementCeiling ceiling;ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;ceiling.stage_top34=20000;ceiling.query_height668=80;ceiling.previous_y30=before.position[1];
        check(character_movement_air_step(p,before,1,1,false,ceiling,&proposed,&error)&&
            gred_vertical_jump_flat_contact(ground,before,proposed,&accepted,&hit,&error),"shared source integration then qualified flat contact candidate");
        check(gred_vertical_jump_finish(plan,false,hit,&state),"contact predicate after source lower motion");
        motion=accepted;maximum=std::max(maximum,motion.position[1]);landed=hit;++falling_updates;
    }
    check(landed&&maximum>50&&motion.position[1]==0&&motion.vertical48==0&&state.selector==7&&state.cached87==1,"release preserves upward momentum through apex, then declares landing; cached87 is still prior callback state");
    check(gred_vertical_jump_plan(p,state,motion,850,0,&plan)&&plan.landing_prefix&&plan.program==8&&plan.state.cached87==0,"landing entry performs takeoff-prefix/landing request and clears cached airborne state");
    check(gred_vertical_jump_finish(plan,true,false,&state)&&state.selector==7,"landing request does not skip completion");
    check(gred_vertical_jump_plan(p,state,motion,850,0,&plan)&&gred_vertical_jump_finish(plan,true,false,&state)&&state.selector==0,"landing VM update completion returns to idle");
    auto changed=p;changed.model=10;
    check(!gred_vertical_jump_plan(changed,{4,0,false},motion,850,0,&plan),"G-Red mapping cannot be generalized to Sasuke");
    motion.speed44=1;check(!gred_vertical_jump_plan(p,{4,0,false},motion,850,0,&plan),"moving takeoff remains refused");
    motion.speed44=0;check(!gred_vertical_jump_plan(p,{5,1,true},motion,850,8u<<16,&plan),"air steering remains refused");
    ground.actor.current24=motion.position[1];ground.actor.offset6d0=1;bool hit=false;
    check(!gred_vertical_jump_flat_contact(ground,motion,motion,&motion,&hit,&error),"unreviewed pose-height offset contact refuses");
    ground.actor.offset6d0=0;ground.actor.attached1db=true;
    check(!gred_vertical_jump_flat_contact(ground,motion,motion,&motion,&hit,&error),"attached ground contact refuses");
    ground.actor.attached1db=false;
    auto sloped=triangle(0);for(auto& vertex:sloped.vertices)vertex[1]=.1f*vertex[0];
    sloped.plane={-.09950372f,.9950372f,0,0};auto slope_data=bytes({sloped});StihGrid slope;
    check(slope.load({slope_data.data(),slope_data.size()},&error),"owned sloped static grid");
    ground.grids[0]=&slope;
    auto above=motion;above.position[1]=100;ground.actor.current24=100;
    check(gred_vertical_jump_flat_contact(ground,above,above,&motion,&hit,&error)&&!hit,"source airborne early return permits overflight without authorizing a sloped response");
    auto into=above;into.position[1]=-30;
    check(!gred_vertical_jump_flat_contact(ground,above,into,&motion,&hit,&error),"source sloped response is not replaced by a fabricated flat landing");
    auto moving=p;moving.walk_operands=true;moving.walk_angle_divisor=32768;moving.air_operands=true;
    moving.air_damping={.98f,.02f,.99f,.01f};moving.ascent_pitch=8192;moving.air_ready_delay=20;
    moving.air[3]=6;moving.air[4]=4;moving.turn_counts.fill(11);moving.turn_gains={.04f,.05f,.02f};
    CharacterMovementMotionState walk;walk.speed44=12;walk.position={-200,0,-200};
    check(gred_vertical_jump_plan(moving,{4,0,false},walk,1000,0x80100,&plan,true)&&plan.motion.speed44==0&&plan.motion.planar_change4c==0,"normal G-Red moving takeoff clears walk speed; no inferred class1 momentum preservation");
    walk.speed44=6;
    check(gred_jump_acceleration(moving,true,&walk)&&walk.speed44==6&&std::abs(walk.planar_change4c-.5656854f)<.00001f&&std::abs(walk.vertical_change50-.5656854f)<.00001f,"directional ascent splits acceleration by original signed16 pitch");
    check(gred_jump_acceleration(moving,false,&walk)&&std::abs(walk.speed44-5.88f)<.00001f&&walk.planar_change4c==0&&walk.vertical_change50==.8f,"released direction damps momentum and retains neutral vertical acceleration");
    walk={};walk.position={-200,100,-200};walk.speed44=8;
    CharacterMovementHeading heading;heading.has_direction=true;heading.desired5ac=16384;
    check(gred_jump_falling_drift(moving,heading,&walk)&&std::abs(walk.position[0]+196)<.00001f&&std::abs(walk.speed44-7.92f)<.00001f&&walk.travel5ae==0,"falling input adds direct translation without rewriting body/travel momentum");
    auto before=walk;before.position={-200,5,-200};auto proposed=before;proposed.position={-196,-1,-200};
    ground.grids[0]=&floor;ground.actor.current24=5;
    check(gred_vertical_jump_flat_contact(ground,before,proposed,&walk,&hit,&error,true)&&hit&&walk.position[0]==-196&&walk.position[1]==0,"moving subset admits only equal-height flat static landing endpoints");
    walk={};walk.flags5e0=0x2000;
    check(!gred_vertical_jump_plan(moving,{5,1,false},walk,1000,0x100,&plan,true),"unreviewed boosted/alternate launch flag refuses rather than taking normal ascent");
    walk.flags5e0=0;
    check(!gred_vertical_jump_plan(moving,{5,1,true},walk,1000,0x100,&plan,true),"moving ascent cannot skip its initialized ordered turn stage");
    std::printf("vertical source jump subset: %u checks PASS; %u falling updates; numeric/flat static candidates, no whole-frame equivalence\n",checks,falling_updates);
}
