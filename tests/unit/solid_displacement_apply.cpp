#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/solid_displacement_apply.hpp"
#include "engine/gameplay/stage_ground_post_solid_floor.hpp"
#include "tests/unit/stage_ground_fixture.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;if(!ok){std::fprintf(stderr,"solid displacement application: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=SolidPairSide;
    SolidDisplacementSnapshot s;s.declared=true;
    for(auto& a:s.slots)a.active00=1;
    SolidPairDisplacementPlan plan;
    plan.count=1;plan.steps[0]={S::First,{3,0,0}};
    SolidDisplacementApplication out;std::string error;
    const std::array<u8,2> owners{0,5};
    auto apply=[&](){return solid_pair_apply_displacements(s,owners,plan,&out,&error);};
    auto order=[&](std::initializer_list<u8> expected){
        check(out.writes==expected.size()&&std::equal(expected.begin(),expected.end(),out.write_order.begin()),"exact source write order");
    };
    s.slots[0].links1dc=0x46; // link slots 1 and 2, high bit outside six slots
    s.slots[1].links1dc=0x8;
    s.slots[2].links1dc=0x8;
    s.slots[3].links1dc=0x10;
    s.slots[3].moved1dd=0x80;
    check(apply(),"shared linked actor is valid in an explicit snapshot");
    order({0,1,2,3,3});
    check(out.snapshot.slots[3].position[0]==6&&out.snapshot.slots[3].accumulated1cc[0]==6&&
        out.snapshot.slots[3].moved1dd==0x81&&out.snapshot.slots[4]==s.slots[4]&&s.slots[3].position[0]==0,
        "preserve both shared-child writes and old bits; no third level or input mutation");
    s.slots[2].active00=0;
    check(apply(),"inactive linked slot is not a first-pass marker");
    order({0,1,3});
    s.slots[1].links1dc=1;
    check(apply(),"second pass can return to the original target without recursion");
    order({0,1,0});

    s.slots[0].links1dc=1;
    check(apply(),"preserve exact fixed-depth order even for a synthetic self link");
    order({0,0,0});
    for(auto& a:s.slots)a.links1dc=0;
    s.slots[0].position[0]=1.25f;s.slots[0].accumulated1cc[0]=65536.f;
    plan.steps[0].delta={.002f,0,0};
    check(apply()&&out.snapshot.slots[0].position[0]==1.25f&&out.snapshot.slots[0].accumulated1cc[0]==65536.f&&
        out.snapshot.slots[0].moved1dd==1,"remove old accumulation and restore updated rounded displacement, not direct position addition");
    s.slots[0].position={};s.slots[0].accumulated1cc={};s.slots[0].links1dc=0x20;
    SolidPairDisplacementInput pair{{1,0,0},-8,{{{2,0,0},{6,0,0}}},0,true};
    check(solid_pair_displacement_plan(pair,&plan,&error)&&apply(),"consume a real ordered two-owner plan");
    order({5,0,5});
    check(out.snapshot.slots[0].position[0]==6&&out.snapshot.slots[5].position[0]==4,"second writes before first and its propagation");
    const auto retained=out;
    auto invalid=plan;invalid.steps[1].delta[0]=1e8f;
    check(!solid_pair_apply_displacements(s,owners,invalid,&out,&error)&&out==retained,"bad second step cannot publish a partial application");
    check(!solid_pair_apply_displacements(s,{0,0},plan,&out,&error)&&out==retained,"owner alias is outside the reviewed scope");
    std::swap(invalid.steps[0],invalid.steps[1]);
    check(!solid_pair_apply_displacements(s,owners,invalid,&out,&error)&&out==retained,"unordered pair refuses");
    auto maximal=s;for(auto& actor:maximal.slots){actor.active00=1;actor.links1dc=0x3f;}
    check(solid_pair_apply_displacements(maximal,owners,plan,&out,&error)&&out.writes==86,
        "the full six-slot two-pass order stays bounded for both writes");
    check(solid_pair_apply_displacements(s,owners,{},&out,&error)&&!out.writes&&out.snapshot==s,
        "no-step plan cannot create contact history");

    // Synthetic composition only: the post-solid query consumes the bit and
    // accumulated displacement from a real write, not from an endpoint guess.
    StihGrid floor,empty,ceiling;
    auto load=[&](StihGrid& grid,const std::vector<StihTriangle>& triangles){
        const auto data=stage_ground_fixture::bytes(triangles);
        check(grid.load({data.data(),data.size()},&error),"synthetic static grids");
    };
    load(floor,{stage_ground_fixture::triangle(10)});load(empty,{});load(ceiling,{stage_ground_fixture::triangle(1000,true)});
    s.slots[0].links1dc=0;s.slots[0].position={-10,11,-10};
    pair.normal={0,1,0};pair.separation=2;pair.owners[1].weight204=50000;
    check(solid_pair_displacement_plan(pair,&plan,&error)&&apply(),"one-owner downward solid write");
    const auto& a=out.snapshot.slots[0];
    check(a.position==std::array<f32,3>{-10,9,-10}&&a.accumulated1cc[1]==-2&&a.moved1dd==1,"applied displacement creates later query inputs");
    StagePostSolidFloorResult floor_result;
    StagePostSolidFloorInput floor_input{{true,0,a.position[1],11,0,0},a.position,a.accumulated1cc,60,true,a.moved1dd,0};
    check(stage_ground_post_solid_floor({&floor,&empty,&ceiling},stage_ground_fixture::parameters(),floor_input,&floor_result,&error)&&
        floor_result.position==std::array<f32,3>{-10,10,-10}&&floor_result.lift==1&&floor_result.hold1df==2,
        "separate detached floor correction raises Y after the solid write");
    std::printf("solid displacement application and bounded linked-slot order: %u checks PASS\n",checks);
}
