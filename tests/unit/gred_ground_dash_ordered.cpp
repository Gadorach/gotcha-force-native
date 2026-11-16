#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include "engine/gameplay/gred_ground_dash.hpp"
#include "engine/gameplay/gred_ground_correction.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ordered ground dash: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile profile(){
    using namespace gf;CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.dimensions.fill(20);p.epsilon=.0001f;p.angle_unit=bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};p.special_min_vertical=-35;
    p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;p.air_dash_operands=true;p.air_dash_windup_factor=.5f;p.air_dash_contact_delay=30;
    p.surface_operands=true;p.contact_motion_epsilon=bits_to_f32(0x2edbe6feu);p.landing_angles={10240,6144};
    p.steep_operands=true;p.steep_damping=.9f;p.relaunch_floor_operands=true;p.relaunch_floor_damping=.8f;
    p.ground_loss_operands=true;p.ground_loss_contact_delay=30;p.ordered_dash_operands=true;p.dash_recovery_damping=.9f;
    for(auto& row:p.ground_dash)row={25,-.5f,22,30};
    p.held_dash_masks={8u<<16,1u<<16,4u<<16,2u<<16};p.air[10]=-1;
    return p;
}
}
int main(){
    using namespace gf;auto p=profile();check(character_movement_profile_valid(p),"owned GFMM9-domain fixture");
    CharacterMovementMotionState motion;motion.position={-200,0,-200};motion.vertical48=7;motion.vertical_change50=8;
    GRedGroundDashOrderedState state;GRedGroundDashOrderedPlan plan;GRedGroundDashOrderedUpdate update;
    GRedGroundDashOrderedResult result;GRedGroundDashPlan legacy;
    check(gred_ground_dash_plan(p,state.dash,0,0,0,&legacy)&&legacy.request_program&&!legacy.move,
        "older live API retains its explicitly legacy request-only entry");
    for(u8 direction=0;direction<4;++direction){
        state.dash.direction_class=direction;
        check(gred_ground_dash_ordered_plan(p,state,motion,0,&plan)&&plan.request_program&&!plan.advance_program&&
            !plan.steer_before_program&&plan.program==27+direction&&plan.state.dash.selector==12&&
            plan.motion.vertical48==0&&plan.motion.vertical_change50==-1&&plan.motion.speed44==25&&
            plan.motion.planar_change4c==-.5f&&plan.state.dash.timer568==30,
            "entry requests directional program, zeros48, copies data6C and loads dash block before motion");
        check(gred_ground_dash_ordered_update(p,plan,3,0,1,nullptr,&update)&&update.steer_after_program&&
            !update.state.exit545&&update.state.dash.timer568==30,
            "entry steers after request; active timer/exit does not run on entry");
    }
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto raw=stage_ground_fixture::bytes(triangles);
        check(grid.load({raw.data(),raw.size()},&error),"bounded synthetic stage");
    };
    load(floor,{stage_ground_fixture::triangle(-80)});load(empty,{});
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.query_height668=200;ground.step674=60;
    CharacterMovementMotionState moved;
    check(character_movement_ground_step(p,update.motion,1,ground,&moved,&error)&&moved.position[1]==0&&
        moved.position[2]==-175&&moved.speed44==24.5f&&moved.vertical48==0,
        "entry670DC moves immediately by25 then decelerates to24.5; large drop holds Y");
    check(gred_ground_dash_ordered_finish(p,update,moved,true,&result)&&result.state.dash.selector==13&&
        result.motion.speed44==24.5f&&!result.idle_reset,"entry selects13 only after motion, ignoring unrelated done");
    GRedGroundCorrectionFields history;history.declared=true;history.previous2c=result.motion.position;
    GRedGroundCorrectionPlan correction;
    check(gred_ground_correction_plan(p,ground,result.motion,history,&correction,&error)&&correction.falling&&
        correction.motion.speed44==24.5f&&correction.motion.position==result.motion.position&&
        correction.falling->lower_program==13,"following priority correction retains POST-motion scalar, not25-unit displacement");
    state=result.state;motion=result.motion;motion.speed44=21.5f;
    for(u8 control=0;control<4;++control){
        check(gred_ground_dash_ordered_plan(p,state,motion,control,&plan)&&plan.steer_before_program&&
            plan.steering_speed44==21.5f&&plan.motion.speed44==22&&plan.motion.planar_change4c==0&&
            plan.advance_program==!(control&1)&&!plan.request_program,
            "active steering sees21.5 before clamp22; VM advance depends on incoming bit1");
        check(gred_ground_dash_ordered_update(p,plan,control,0,1,nullptr,&update)&&!update.steer_after_program&&
            update.state.dash.timer568==(control?29.f:30.f)&&update.state.exit545==bool(control),
            "post-VM nonzero control gates timer and release, independent of pre-VM bit1");
        check(character_movement_ground_step(p,update.motion,1,ground,&moved,&error)&&moved.position[2]==-153&&moved.speed44==22,
            "active still moves on exit frame, after minimum clamp");
        check(gred_ground_dash_ordered_finish(p,update,moved,false,&result)&&result.state.dash.selector==(control?14:13)&&
            !result.state.exit545&&!result.state.recovery546,"active selects14/clears selector fields only after motion");
    }
    motion.speed44=22;state.dash.timer568=.5f;
    check(gred_ground_dash_ordered_plan(p,state,motion,0,&plan)&&plan.motion.planar_change4c==-.5f&&
        gred_ground_dash_ordered_update(p,plan,1,p.held_dash_masks[state.dash.coarse57d],1,nullptr,&update)&&
        update.state.exit545&&update.state.dash.timer568==-.5f&&
        character_movement_ground_step(p,update.motion,1,ground,&moved,&error)&&moved.speed44==21.5f,
        "equal minimum still decelerates; timer expiry latches before this final active motion");
    //1D9B==0 does not make the shared root evaluator's1DAC zero.
    CharacterMotionRootFields root_fields;root_fields.delta5f4=1;root_fields.apply_position1d9b=0;
    CharacterMotionRootObservation before,after;after.translation={0,0,23};after.frame=1;
    CharacterMotionRootState root;
    check(character_motion_root_step(before,after,CharacterMotionEvaluation::Advance,root_fields,{},&root,&error)&&
        root.speed1dac==23&&root.position==CharacterMotionVector{}&&root.updates==1,
        "shared root evaluator derives speed23 without applying actor position");
    state={};state.dash.selector=14;motion={};motion.position={-200,0,-200};motion.speed44=25;motion.planar_change4c=-.5f;motion.vertical_change50=-1;
    for(u8 control:{u8(2),u8(3)}){
        check(!gred_ground_dash_ordered_plan(p,state,motion,control,&plan),
            "first recovery bit2 still refuses unqualified4CFF4 forced continuation");
    }
    check(gred_ground_dash_ordered_plan(p,state,motion,0,&plan)&&plan.advance_program&&!plan.steer_before_program,
        "recovery first advances the existing program, not a guessed clip");
    update.motion.speed44=123;
    check(!gred_ground_dash_ordered_update(p,plan,0,0,1,nullptr,&update)&&update.motion.speed44==123,
        "absent post-VM root state refuses without output mutation");
    auto bad_root=root;bad_root.updates=0;
    check(!gred_ground_dash_ordered_update(p,plan,0,0,1,&bad_root,&update),"uninitialized root state refuses");
    bad_root=root;bad_root.speed1dac=std::numeric_limits<f32>::quiet_NaN();
    check(!gred_ground_dash_ordered_update(p,plan,0,0,1,&bad_root,&update),"nonfinite root speed refuses");
    check(gred_ground_dash_ordered_update(p,plan,3,0,1,&root,&update)&&update.motion.speed44==23&&
        update.state.recovery546==1&&update.steer_after_program&&update.clear_control1d0f&&update.state.exit545,
        "recovery damps25*.9, clamps strictly below root23, latches546 and clears control3 once");
    check(character_movement_ground_step(p,update.motion,1,ground,&moved,&error)&&moved.position[2]==-177&&moved.speed44==22.5f&&
        gred_ground_dash_ordered_finish(p,update,moved,false,&result)&&result.state.dash.selector==14&&result.motion.speed44==22.5f,
        "recovery motion uses23 before -.5 update; not rendered/previous-frame speed");
    state=result.state;motion=result.motion;root.speed1dac=10;
    check(gred_ground_dash_ordered_plan(p,state,motion,3,&plan)&&plan.advance_program&&
        gred_ground_dash_ordered_update(p,plan,3,0,1,&root,&update)&&update.motion.speed44==10&&
        !update.clear_control1d0f&&update.state.exit545,
        "continued recovery follows new root speed exactly and does not repeat latched control clearing");
    check(character_movement_ground_step(p,update.motion,1,ground,&moved,&error)&&moved.position[2]==-167&&moved.speed44==9.5f&&
        gred_ground_dash_ordered_finish(p,update,moved,true,&result)&&result.idle_reset&&result.state.dash.selector==0&&
        result.motion.position==moved.position&&result.motion.speed44==0&&result.motion.planar_change4c==0,
        "done frame moves by10 BEFORE reset; final idle must not erase that motion");
    check(result.motion.vertical48==0&&result.motion.vertical_change50==0,"common idle reset also clears48/50 after movement");
    state={};state.dash.selector=14;motion.speed44=20;root.speed1dac=f32(20*p.dash_recovery_damping);
    check(gred_ground_dash_ordered_plan(p,state,motion,0,&plan)&&
        gred_ground_dash_ordered_update(p,plan,0,0,1,&root,&update)&&!update.state.recovery546&&
        update.motion.speed44==root.speed1dac,"equality does not latch546: source comparison is strict");
    root.speed1dac=0;motion.speed44=0;
    check(gred_ground_dash_ordered_plan(p,state,motion,0,&plan)&&
        gred_ground_dash_ordered_update(p,plan,0,0,1,&root,&update)&&!update.steer_after_program,
        "zero recovery speed does not steer body toward camera");
    auto distinct=p;distinct.dash_recovery_damping=.5f;motion.speed44=20;
    check(gred_ground_dash_ordered_plan(distinct,state,motion,0,&plan)&&
        gred_ground_dash_ordered_update(distinct,plan,0,0,1,&root,&update)&&update.motion.speed44==10,
        "recovery uses its separately owned operand, never the numerically similar steep factor");
    update.motion.speed44=123;
    for(f32 delta:{-1.f,17.f,std::numeric_limits<f32>::infinity(),std::numeric_limits<f32>::quiet_NaN()}){
        check(!gred_ground_dash_ordered_update(p,plan,0,0,delta,&root,&update)&&update.motion.speed44==123,
            "invalid clock refuses without updating candidate");
    }
    check(!gred_ground_dash_ordered_plan(p,state,motion,0,nullptr)&&
        !gred_ground_dash_ordered_update(p,plan,0,0,1,&root,nullptr)&&
        !gred_ground_dash_ordered_finish(p,update,moved,false,nullptr),"null candidate outputs refuse");
    auto refused=[&](CharacterMovementProfile profile,GRedGroundDashOrderedState s,CharacterMovementMotionState m,const char* why){
        plan.motion.speed44=123;check(!gred_ground_dash_ordered_plan(profile,s,m,0,&plan)&&plan.motion.speed44==123,why);
    };
    auto old=p;old.ordered_dash_operands=false;refused(old,state,motion,"GFMM8 equal numeric damping cannot grant ordered-dash operands");
    old=p;old.class_flags=8;refused(old,state,motion,"not generalized to other movement classes");
    old=p;old.movement_type=2;refused(old,state,motion,"movement-type2 recovery path is not approximated");
    old=p;old.dash_recovery_damping=0;refused(old,state,motion,"invalid new operand refuses");
    auto unknown=state;unknown.recovery546=2;refused(p,unknown,motion,"unknown recovery phase refuses");
    unknown=state;unknown.recovery546=1;refused(p,unknown,motion,"new recovery cannot skip selector-cleared546");
    unknown=state;unknown.dash.selector=0;refused(p,unknown,motion,"idle is not an ordered dash input");
    auto blocked=motion;blocked.flags5e0=0x40;refused(p,state,blocked,"air-dash/contact caller branch remains outside ground subset");
    blocked.flags5e0=0x2000;refused(p,state,blocked,"blocked-motion flags refuse");
    blocked=motion;blocked.speed44=-1;refused(p,state,blocked,"negative scalar is outside normal dash progression");
    std::ifstream file("local/content/source-gred-ground-v9/motion.gfmm",std::ios::binary);
    std::vector<u8> bytes{std::istreambuf_iterator<char>(file),{}};
    if(!bytes.empty()){
        CharacterMovementProfile real;
        check(character_movement_load({bytes.data(),bytes.size()},&real,&error)&&real.ordered_dash_operands&&
            real.dash_recovery_damping==bits_to_f32(0x3f666666u)&&real.ground_loss_operands,
            "qualified GFMM9 preserves recovery80437570 separately from contact804375D4");
        for(std::size_t size=0;size<bytes.size();++size){
            check(!character_movement_load({bytes.data(),size},&real,&error)&&!real.loaded,"all truncated GFMM9 extents refuse");
        }
        for(unsigned i=0;i<bytes.size();++i){
            auto edited=bytes;edited[i]^=1;
            check(!character_movement_load({edited.data(),edited.size()},&real,&error)&&!real.loaded,"every edited GFMM9 byte refuses");
        }
    }
    std::printf("ordered G-Red dash entry/active/root-recovery/tail: %u checks PASS; numeric candidates only, no live dash-ledge permission\n",checks);
}
