#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/gred_air_dash_contact.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"air-dash floor: %s (%s)\n",why,error.c_str());std::abort();}}
gf::CharacterMovementProfile profile(){
    gf::CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;p.rank.fill(1);p.dimensions.fill(20);
    p.epsilon=.0001f;p.angle_unit=gf::bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};p.special_min_vertical=-35;
    p.walk_operands=true;p.walk_angle_divisor=32768;p.air_operands=true;p.air_dash_operands=true;p.air_dash_windup_factor=.5f;p.air_dash_contact_delay=30;
    p.surface_operands=true;p.landing_angles={10240,6144};p.contact_motion_epsilon=gf::bits_to_f32(0x2edbe6feu);
    p.steep_operands=true;p.steep_damping=gf::bits_to_f32(0x3f666666u);
    p.relaunch_floor_operands=true;p.relaunch_floor_damping=gf::bits_to_f32(0x3f4ccccdu);p.air[11]=gf::bits_to_f32(0xbd8f5c29u);return p;
}
}
int main(){
    using namespace gf;auto p=profile();check(character_movement_profile_valid(p),"owned GFMM7-domain numeric fixture");
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto raw=stage_ground_fixture::bytes(triangles);check(grid.load({raw.data(),raw.size()},&error),"owned bounded static grid");
    };
    load(floor,{stage_ground_fixture::triangle(0)});load(empty,{});
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=5;ground.query_height668=80;ground.step674=30;
    CharacterMovementMotionState before;before.position={-200,5,-200};before.speed44=8;before.vertical48=-10;
    auto proposed=before;proposed.position[1]=-5;proposed.vertical48=-10.07f;proposed.vertical_change50=p.air[11];proposed.planar_change4c=-.2f;
    for(f32 speed:{-10.07f,.1f})for(u8 selector:{u8(15),u8(16)}){
        proposed.vertical48=speed;GRedSurfaceContact contact;
        check(gred_air_dash_static_contact(p,ground,before,proposed,selector,false,true,&contact,&error)&&
            contact.kind==GRedSurfaceContactKind::Snapped&&contact.motion.position[1]==0&&
            contact.motion.vertical48==(selector==15?0:speed)&&contact.motion.vertical_change50==p.air[11]&&
            contact.motion.speed44==proposed.speed44&&contact.motion.planar_change4c==proposed.planar_change4c,
            "null callback snaps height; wind-up unconditionally clears48, active preserves48, neither resets50 or applies .8");
    }
    GRedSurfaceContact contact;
    check(gred_air_dash_static_contact(p,ground,before,proposed,16,true,true,&contact,&error)&&
        contact.kind==GRedSurfaceContactKind::Landed&&contact.motion.vertical48==0&&contact.motion.vertical_change50==0,
        "installed65E64 remains a real landing response, not an active null-callback snap");
    const auto retained=contact.motion.position;
    check(!gred_air_dash_static_contact(p,ground,before,proposed,15,true,true,&contact,&error)&&contact.motion.position==retained,
        "wind-up cannot claim an installed landing callback");
    check(!gred_air_dash_static_contact(p,ground,before,proposed,17,false,true,&contact,&error),
        "recovery uses its separate callback path");
    auto older=p;older.relaunch_floor_operands=false;
    check(!gred_air_dash_static_contact(older,ground,before,proposed,15,false,true,&contact,&error),
        "older profile does not widen existing null-callback candidate permission");
    for(auto y:{10.f,6.f}){
        proposed.position[1]=y;proposed.vertical48=-9;
        check(gred_air_dash_static_contact(p,ground,before,proposed,15,false,true,&contact,&error)&&
            contact.kind==GRedSurfaceContactKind::Above&&contact.motion.vertical48==-9,
            "ret0 above contact does not trigger wind-up's ret1-only zero");
    }
    before.position[1]=-5;proposed.position[1]=-4;ground.actor.current24=ground.actor.previous30=-5;
    check(gred_air_dash_static_contact(p,ground,before,proposed,15,false,true,&contact,&error)&&
        contact.kind==GRedSurfaceContactKind::Leaving&&contact.motion.vertical48==-9,
        "moving away from the surface is ret0, not a floor snap or vertical zero");
    auto steep=stage_ground_fixture::triangle(0);const auto length=std::sqrt(10.f);
    for(auto& v:steep.vertices)v[1]=-3*v[0];
    steep.plane={3/length,1/length,0,0};load(floor,{steep});
    before.position[1]=605;proposed=before;proposed.position[1]=595;proposed.vertical48=-10;proposed.vertical_change50=p.air[11];
    ground.actor.current24=ground.actor.previous30=605;
    for(u8 selector:{u8(15),u8(16)}){
        check(gred_air_dash_static_contact(p,ground,before,proposed,selector,false,true,&contact,&error)&&
            contact.kind==GRedSurfaceContactKind::Pushed&&contact.motion.vertical48==-9&&contact.motion.speed44==7.2f&&
            contact.motion.vertical_change50==p.air[11],
            "steep projection is ret0: preserve shared .9 push instead of caller floor zero or .8 response");
        check(!gred_air_dash_static_contact(p,ground,before,proposed,selector,false,false,&contact,&error),
            "caller push opt-in remains required");
    }
    auto slope=stage_ground_fixture::triangle(0);const f32 diagonal=std::sqrt(2.f);
    for(auto& v:slope.vertices)v[1]=-v[0];
    slope.plane={1/diagonal,1/diagonal,0,0};load(floor,{slope});
    before.position[1]=205;proposed=before;proposed.position[1]=195;proposed.vertical48=-10;
    proposed.vertical_change50=p.air[11];ground.actor.current24=ground.actor.previous30=205;
    for(u8 selector:{u8(15),u8(16)}){
        check(gred_air_dash_static_contact(p,ground,before,proposed,selector,false,false,&contact,&error)&&
            contact.kind==GRedSurfaceContactKind::Snapped&&contact.angle>6144&&contact.angle<=10240,
            "default declared-zero timer preserves existing AC floor contract");
        const auto prior=contact.motion.position;
        check(!gred_air_dash_static_contact(p,ground,before,proposed,selector,false,false,&contact,&error,7)&&contact.motion.position==prior,
            "explicit positive698 reaches shared AE gate, not a default-zero substitution");
    }
    check(gred_air_dash_static_contact(p,ground,before,proposed,16,true,false,&contact,&error,0)&&contact.kind==GRedSurfaceContactKind::Landed,
        "explicit zero698 callback uses original AC landing gate");
    check(!gred_air_dash_static_contact(p,ground,before,proposed,16,true,false,&contact,&error,7),
        "positive698 callback also uses stricter AE gate");
    check(!gred_air_dash_static_contact(p,ground,before,proposed,15,false,false,&contact,&error,-1),
        "caller cannot supply an invalid timer");
    ground.actor.attached1db=1;
    check(!gred_air_dash_static_contact(p,ground,before,proposed,15,false,true,&contact,&error),
        "attachment and moving-platform domain remains refused");
    std::printf("air-dash static caller tails: %u checks PASS; numeric candidates only, no native actor or original whole-frame authority\n",checks);
}
