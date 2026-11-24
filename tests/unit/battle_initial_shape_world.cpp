#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_shape_world.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> bytes{};const std::array<u32,32> words{0x47465357,1,128,0,0x8001c5ac,0x8003f688,0x8003e0a8,0x80045ef4,0x800462e4,0x8020b4f8,0x8020af5c,0x8020bc88,0x80055dcc,0x8003cc24,0x80089554,0x802d0d44,0,4,0x2edbe6fe,0xbdf8e542,0,0x3f800000,0x40000000,0x42c80000,0xc59c4000,0x80437080,0,1,0,19,16,0};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialShapeWorldProfile profile,bad;check(profile.load({bytes.data(),bytes.size()},&error));
    for(unsigned at=0;at<bytes.size();++at){auto edited=bytes;edited[at]^=1;check(!bad.load({edited.data(),edited.size()},&error));}
    StihGrid floor,ceil,other;auto f=stage_ground_fixture::bytes({stage_ground_fixture::triangle(0)}),c=stage_ground_fixture::bytes({stage_ground_fixture::triangle(1000,true)}),e=stage_ground_fixture::bytes({});
    check(floor.load({f.data(),f.size()},&error)&&ceil.load({c.data(),c.size()},&error)&&other.load({e.data(),e.size()},&error));
    const auto p=stage_ground_fixture::parameters();const std::array<const StihGrid*,3> grids{&floor,&other,&ceil};StageGroundActor actor{true,0,50,50,0,0};Mat4 bone;bone.at(0,3)=-10;bone.at(1,3)=300;bone.at(2,3)=-10;
    BattleInitialShapeWorld world;check(battle_initial_shape_world(profile,grids,p,actor,0,bone,{1,-1,1},30,50,1,1,&world,&error));
    check(world.position20==std::array<f32,3>{-10,0,-10}&&world.normal38==std::array<f32,3>{0,1,0}&&world.surface.selected&&world.distance_squared==90000&&world.range_squared==25000000);
    check(world.visibility144==f32(1.f-f32(90000.f/25000000.f))&&world.world114.at(0,3)==-10&&world.world114.at(1,3)==0);
    BattleInitialShapeWorld larger;check(battle_initial_shape_world(profile,grids,p,actor,0,bone,{1,-1,1},30,50,1,2,&larger,&error)&&larger.visibility144==world.visibility144&&larger.base_scale==100);
    check(!battle_initial_shape_world(profile,grids,p,actor,1,bone,{1,-1,1},30,50,1,1,&larger,&error));
    auto attached=actor;attached.attached1db=1;check(!battle_initial_shape_world(profile,grids,p,attached,0,bone,{1,-1,1},30,50,1,1,&larger,&error));
    check(!battle_initial_shape_world(profile,grids,p,actor,0,bone,{0,1,0},30,50,1,1,&larger,&error)); //source unwritten-basis branch, never identity fallback
    StageGroundSurfaceResult surface;const std::array<f32,3> outside{8000,123,0};f32 old=0;
    check(stage_ground_surface(grids,p,actor,outside,30,&surface,&error)&&surface.height==-5000&&!surface.inside);
    check(stage_ground_height(grids,p,actor,outside,30,&old,&error)&&old==123); //old sibling remains unchanged
    for(unsigned n=0;n<bytes.size();++n)check(!bad.load({bytes.data(),n},&error));
    std::printf("static shape surface/basis/scale/fade source branch: %u checks PASS\n",checks);
}
