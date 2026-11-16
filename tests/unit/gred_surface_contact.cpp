#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_surface_contact.hpp"
#include "engine/gameplay/gred_movement_admission.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source surface contact: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile profile(){
    gf::CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;p.rank.fill(1);p.dimensions.fill(20);
    p.epsilon=.0001f;p.angle_unit=gf::bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};p.special_min_vertical=-35;
    p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;p.air_dash_operands=true;p.air_dash_windup_factor=.5f;p.air_dash_contact_delay=30;
    p.surface_operands=true;p.landing_angles={10240,6144};p.contact_motion_epsilon=gf::bits_to_f32(0x2edbe6feu);return p;
}
gf::StihTriangle tilted(float intercept,float slope){
    auto t=stage_ground_fixture::triangle(intercept);for(auto& v:t.vertices)v[1]=intercept+slope*v[0];
    const float n=std::sqrt(1+slope*slope);t.plane={-slope/n,1/n,0,-intercept/n};return t;
}
}
int main(){
    using namespace gf;auto p=profile();check(character_movement_profile_valid(p),"owned normal-G-Red GFMM5-domain fixture");
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto b=stage_ground_fixture::bytes(triangles);check(grid.load({b.data(),b.size()},&error),"owned static grid");
    };
    load(empty,{});load(floor,{tilted(10,.25f),stage_ground_fixture::triangle(80)});
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.query_height668=80;ground.step674=30;
    CharacterMovementMotionState before,proposed;before.position={-200,300,-200};proposed=before;proposed.position[1]=-200;proposed.vertical48=-30;
    ground.actor.current24=ground.actor.previous30=before.position[1];CharacterMotionVector independent;
    check(ground.normal(proposed.position,&independent,&error)&&independent==CharacterMotionVector{0,1,0},"independent normal sibling has source defaultUP on this lowest-floor fallback");
    GRedSurfaceContact contact;
    check(gred_surface_contact(p,ground,before,proposed,{},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Landed&&contact.surface.selected&&
        contact.surface.triangle==0&&contact.surface.normal[0]<0&&contact.angle>0&&std::abs(contact.motion.position[1]+40)<.001f&&contact.motion.vertical48==0,
        "paired height+normal preserves lowest-floor winner instead of fabricating flat contact (numeric query, not a live500-unit motion offer)");
    load(floor,{tilted(0,1)});before.position={-200,-190,-200};proposed=before;proposed.position[1]=-205;
    ground.actor.current24=ground.actor.previous30=before.position[1];
    check(gred_surface_contact(p,ground,before,proposed,{},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Landed&&std::abs(int(contact.angle)-8192)<=1&&
        std::abs(contact.motion.position[1]+200)<.001f,"normal classAC angle admits45-degree selected surface");
    const auto retained=contact.motion.position;
    check(!gred_surface_contact(p,ground,before,proposed,{1,0},&contact,&error)&&contact.motion.position==retained,
        "positive timer698 selects stricter classAE angle and refuses unreviewed steep response atomically");
    load(floor,{tilted(0,3)});before.position[1]=-590;proposed=before;proposed.position[1]=-605;
    ground.actor.current24=ground.actor.previous30=before.position[1];
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error),"steep push/damping branch is not replaced by unconditional landing");
    check(!gred_surface_contact(p,ground,before,proposed,{0,0,true},&contact,&error),"push opt-in still requires GFMM6");
    p.steep_operands=true;p.steep_damping=bits_to_f32(0x3f666666u);
    proposed.speed44=10;proposed.vertical48=-20;proposed.planar_change4c=2;proposed.vertical_change50=-1;
    check(gred_surface_contact(p,ground,before,proposed,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
        contact.motion.speed44==9&&contact.motion.vertical48==-18&&contact.motion.planar_change4c==0&&contact.motion.vertical_change50==-1&&
        contact.motion.position[0]<proposed.position[0]&&contact.motion.position[1]>proposed.position[1]&&contact.motion.position[1]<contact.surface.height,
        "steep scale/add clears4C and damps44/48 only; no landing snap or50 reset");
    CharacterMotionVector projected;
    check(source_surface_push_position(proposed.position,contact.surface.normal,contact.surface.height,contact.angle,&projected)&&projected==contact.motion.position,
        "shared quantized cosine projection matches; selected normal is not normalized or replaced");
    s16 unnormalized_angle=0;f32 cosine=0;
    check(source_surface_angle({3,1,0},&unnormalized_angle),"source angle uses direction, not normal magnitude");
    source_heading_xz(unnormalized_angle,nullptr,&cosine);const f32 scale=f32(5*cosine);
    check(source_surface_push_position({0,-5,0},{3,1,0},0,unnormalized_angle,&projected)&&
        projected==CharacterMotionVector{f32(3*scale),f32(-5+scale),0},
        "independent nonunit-normal fixture preserves raw vector scale/add, without renormalization or snap");
    const auto push_position=contact.motion.position;
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error)&&contact.motion.position==push_position,"GFMM6 alone cannot widen older surface route");
    load(floor,{stage_ground_fixture::triangle(0)});before.position={-200,-2,-200};proposed=before;proposed.position[1]=-1;
    ground.actor.current24=ground.actor.previous30=-2;
    check(gred_surface_contact(p,ground,before,proposed,{},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Leaving&&contact.motion.position==proposed.position,
        "positive displacement dot is a source no-contact early return, not permission for native below-floor motion");
    before.position[1]=100;proposed=before;proposed.position[1]=80;ground.actor.current24=ground.actor.previous30=100;
    check(gred_surface_contact(p,ground,before,proposed,{},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Above,"above-height early return remains separate from landing");
    check(!gred_surface_contact(p,ground,before,proposed,{0,1},&contact,&error),"unreviewed source7C9/suspended-contact state refuses");
    ground.grids[0]=&empty;
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error),"missing selected surface cannot become guessedUP floor");
    ground.grids[0]=&floor;ground.actor.attached1db=1;
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error),"attached/moving-platform branch refuses");
    ground.actor.attached1db=0;ground.actor.offset6d0=1;
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error),"unreviewed pose-offset response refuses");
    ground.actor.offset6d0=0;p.surface_operands=false;
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error),"legacy operands cannot authorize slope contact");
    p.surface_operands=true;p.model=10;
    check(!gred_surface_contact(p,ground,before,proposed,{},&contact,&error),"G-Red angle gates are not generalized to Sasuke");
    CharacterMovementInputProfile input_profile;input_profile.loaded=true;input_profile.model=0x615;input_profile.model_hash=123;
    input_profile.held_mask=0xf0100;input_profile.resource_policy=1;input_profile.low_threshold=20;input_profile.high_threshold=52;
    input_profile.axis_window=2;input_profile.other_window=3;input_profile.direction_window=20;input_profile.release_window=16;
    input_profile.octants.fill(1);input_profile.resource_limit=10000;
    for(auto& c:input_profile.commands){c.kind=1;c.pulse=1;c.masks={1,0};}
    CharacterMovementInputState input;input.previous_held=0x80100;input.commands5d4=3;input.directions5d8=0x80;
    CharacterAirResourceState resource;resource.value736=10000;GRedMovementAdmissionFields fields;fields.selector=4;fields.prior_handlers_declined=true;
    GRedMovementAdmission admission;
    check(gred_movement_admit(input_profile,input,resource,fields,&admission,&error)&&!admission.changed&&admission.selector==4,
        "source takeoff samples buffered jump/relaunch/flick intent but admits no locomotion cancellation");
    std::printf("paired static G-Red surface/angle/away gates: %u checks PASS; numeric candidates, no whole-frame or hardware equivalence\n",checks);
}
