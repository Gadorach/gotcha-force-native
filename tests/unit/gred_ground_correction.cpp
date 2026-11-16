#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/gameplay/gred_ground_correction.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground correction: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile profile(){
    using namespace gf;CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.dimensions.fill(20);p.epsilon=.0001f;p.angle_unit=bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};p.special_min_vertical=-35;
    p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;p.air_dash_operands=true;p.air_dash_windup_factor=.5f;p.air_dash_contact_delay=30;
    p.surface_operands=true;p.contact_motion_epsilon=bits_to_f32(0x2edbe6feu);p.landing_angles={10240,6144};
    p.steep_operands=true;p.steep_damping=.9f;p.relaunch_floor_operands=true;p.relaunch_floor_damping=.8f;
    p.ground_loss_operands=true;p.ground_loss_contact_delay=30;return p;
}
struct Drop final:gf::IBattleMvpTerrain{
    bool height(float x,float,float,float* out)const override{*out=x<0?80:0;return true;}
};
}
int main(){
    using namespace gf;auto p=profile();check(character_movement_profile_valid(p),"owned GFMM8-domain numeric profile");
    StihGrid floor,empty;auto load=[&](StihGrid& g,std::vector<StihTriangle> t){
        const auto raw=stage_ground_fixture::bytes(t);check(g.load({raw.data(),raw.size()},&error),"owned bounded grid");
    };
    load(empty,{});load(floor,{stage_ground_fixture::triangle(-80)});
    CharacterMovementMotionState before;before.position={-200,0,-200};before.speed44=25;before.planar_change4c=-.5f;before.flags5e0=0x10;
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=0;ground.query_height668=200;ground.step674=30;
    CharacterMovementMotionState departed;
    check(character_movement_ground_step(p,before,1,ground,&departed,&error)&&departed.position[1]==0&&departed.position[2]>before.position[2]&&departed.speed44==24.5f,
        "670DC's large drop preserves Y and horizontal movement, not instant falling dispatch");
    GRedGroundCorrectionFields fields;fields.declared=true;fields.previous2c=departed.position;
    ground.actor.current24=ground.actor.previous30=departed.position[1];GRedGroundCorrectionPlan plan;
    check(gred_ground_correction_plan(p,ground,departed,fields,&plan,&error)&&plan.kind==GRedGroundCorrectionKind::Falling&&
        plan.queried&&plan.queried_height==-80&&plan.height_difference==-80&&plan.motion.position==departed.position&&
        plan.motion.speed44==24.5f&&plan.motion.flags5e0==0&&plan.falling&&plan.falling->selector==6&&
        plan.falling->fall_marker590==1&&plan.falling->lower_program==13&&plan.falling->timer690==0&&
        plan.falling->timer698==30&&plan.falling->clear71b,
        "next correction restores incoming Y, chooses source marker1/program13 and timer698=30 without inventing previous-frame momentum");
    check(plan.motion.vertical48==departed.vertical48&&plan.motion.vertical_change50==departed.vertical_change50,
        "6A5A4 does not perform the later lower falling callback's velocity/acceleration work");
    before=departed;before.position={-197,0,-196};before.speed44=2;before.travel5ae=1234;before.vertical48=6;before.vertical_change50=.25f;
    fields.previous2c={-200,0,-200};
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.motion.speed44==5&&
        plan.motion.vertical48==6&&plan.motion.vertical_change50==.25f,"explicit current-call3/4 history can reconstruct speed5 without changing vertical operands");
    s16 heading=0;check(source_heading_s16(3,4,&heading)&&plan.motion.travel5ae==heading,"history direction writes travel5AE, not Borg body heading");
    before.speed44=5;
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.motion.travel5ae==1234,"equal speed does not replace retained heading");
    before.speed44=6;
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.motion.speed44==6&&plan.motion.travel5ae==1234,
        "smaller measured speed cannot erase current momentum");
    auto ranked=p;ranked.rank[2]=2;before.speed44=1;
    check(gred_ground_correction_plan(ranked,ground,before,fields,&plan,&error)&&plan.motion.speed44==2.5f,"history divides by declared5F8 rank, not a guessed frame delta");
    before.position={-200,0,-200};before.speed44=5;fields.previous2c=before.position;
    for(const auto level:{-30.f,std::nextafter(-30.f,-INFINITY),30.f,std::nextafter(30.f,INFINITY)}){
        load(floor,{stage_ground_fixture::triangle(level)});
        check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error),"strict step boundary candidate");
        check(bool(plan.falling)==(level< -30),"only strict below-minus-step admits falling");
        if(!plan.falling)check(plan.kind==GRedGroundCorrectionKind::Height&&plan.motion.position[1]==level,
            "same X/Z never triggers upward rewind, even above step limit");
    }
    load(floor,{stage_ground_fixture::triangle(31)});before.position={-197,0,-196};
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.kind==GRedGroundCorrectionKind::Rewound&&
        plan.motion.position==fields.previous2c&&!plan.falling,"strict upward step with moved X/Z restores all previous coordinates, no selector change");
    load(floor,{stage_ground_fixture::triangle(-15)});before.position[1]=20;ground.actor.current24=20;
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&!plan.falling&&plan.height_difference==-15&&plan.motion.position[1]==-15,
        "height comparison uses previous30, not incoming24");
    load(floor,{stage_ground_fixture::triangle(50)});before.position[1]=-10;fields.previous2c[1]=100;
    ground.actor.current24=-10;ground.actor.previous30=100;
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.falling&&plan.motion.position[1]==-10,
        "drop restores incoming24, not previous30; numeric candidate is not native geometry authority");
    before.position={-200,0,-200};before.speed44=5;before.flags5e0=0x10;fields.previous2c=before.position;
    ground.actor.current24=ground.actor.previous30=0;load(floor,{stage_ground_fixture::triangle(-80)});
    before.flags5e0=0x40;
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.kind==GRedGroundCorrectionKind::Skipped&&!plan.queried&&
        !plan.falling&&plan.motion.position==before.position,"airborne flag guard skips correction, not a fabricated floor query");
    before.flags5e0=0x10;fields.state4a0=2;
    check(gred_ground_correction_plan(p,ground,before,fields,&plan,&error)&&plan.kind==GRedGroundCorrectionKind::Skipped&&!plan.queried,
        "source4A0=2 guard remains separate from a failed query");
    fields.state4a0=0;const auto retained=plan.motion.position;
    auto refuses=[&](const CharacterMovementProfile& profile,const CharacterMovementStaticGround& g,const GRedGroundCorrectionFields& f,const char* why){
        check(!gred_ground_correction_plan(profile,g,before,f,&plan,&error)&&plan.motion.position==retained,why);
    };
    auto old=p;old.ground_loss_operands=false;refuses(old,ground,fields,"GFMM7 cannot authorize new ground-correction operands");
    auto bad=fields;bad.declared=false;refuses(p,ground,bad,"previous2C/30/34 history must be explicit");
    bad=fields;bad.force732=1;refuses(p,ground,bad,"unowned forced732 producer remains refused");
    bad=fields;bad.override1d9b=1;refuses(p,ground,bad,"stored1D9B momentum override is not guessed");
    bad=fields;bad.previous2c[1]=1;refuses(p,ground,bad,"history and query previous30 must match exactly");
    bad=fields;bad.previous2c[0]=NAN;refuses(p,ground,bad,"nonfinite history refuses atomically");
    auto attached=ground;attached.actor.attached1db=1;refuses(p,attached,fields,"attachments/platforms remain unsupported");
    attached=ground;attached.actor.offset6d0=1;refuses(p,attached,fields,"nonzero pose-height offset remains unsupported");
    attached=ground;attached.grids[2]=nullptr;refuses(p,attached,fields,"all three qualified static grids remain required");
    before.flags5e0=3;refuses(p,ground,fields,"blocked/effect branch of6A5A4 is not silently acknowledged");before.flags5e0=0x10;
    old=p;old.model=10;refuses(old,ground,fields,"G-Red correction is not generalized to Sasuke");
    Drop drop;NativeBattleMvp world;BattleMvpConfig config;config.spawn={BattlePoint{-10,0},BattlePoint{400,400}};
    check(world.start(config,&drop),"owned numeric native ledge, not original actor construction");
    auto producer=world.new_motion_producer(NativeBorgMotionPolicy::GroundOnly);NativeBorgMotionObservation observed;
    check(producer&&world.observe_motion(&observed),"existing ground-only producer");
    NativeBorgMotionOffer offer{observed,observed.position,observed.facing,BattleMvpAction::Idle,1,producer};
    check(world.step({},false,&drop,nullptr,nullptr,&offer)&&world.observe_motion(&observed),"owned standing source proxy");
    offer={observed,observed.position,observed.facing,BattleMvpAction::Move,2,producer};offer.proposed_position[0]=10;
    const auto tick=world.tick();
    check(p.ground_loss_operands&&!world.step({},false,&drop,nullptr,nullptr,&offer)&&world.tick()==tick&&
        world.actors()[0].position==observed.position&&!world.step({},false,&drop),
        "GFMM8 numeric planning grants no native ledge receipt, tick mutation or legacy movement fallback");
    const std::filesystem::path path="local/content/source-gred-ground-v8/motion.gfmm";std::error_code ec;
    const auto size=std::filesystem::file_size(path,ec);
    if(!ec){
        check(size==372,"qualified local GFMM8 extent");std::ifstream f(path,std::ios::binary);std::vector<u8> raw(std::istreambuf_iterator<char>(f),{});
        CharacterMovementProfile actual;
        check(character_movement_load({raw.data(),raw.size()},&actual,&error)&&actual.ground_loss_operands&&
            actual.ground_loss_zero==0&&actual.ground_loss_contact_delay==30,"qualified separate8043762C/80437648 operands");
    }
    std::printf("G-Red ground correction/history/priority operands: %u checks PASS; numeric candidates only, legacy ground-only ledge exit remains refused\n",checks);
}
