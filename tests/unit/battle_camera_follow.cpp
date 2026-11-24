#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_camera_follow.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"camera follow: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;std::array<u8,160> bytes{};
    const std::array<u32,40> words{0x47465646,1,160,0,0x8000bda4,0x8000c918,0x8000cb8c,0x8000cdc0,0x8000fc2c,0x800101c8,0x8001063c,0x80010664,0x8003dd34,0x8000fffc,0x8000c314,0x800452e4,0x800452a0,0x8004526c,0,0x3f800000,0x3f000000,0x3a83126f,0x38c90fdb,0x4622f983,0x40800000,0x40a00000,0x41100000,0x41200000,0x3fa66666,0x3f4ccccd,0x3fc00000,0x3e99999a,0x40400000,0x3e800000,1536,12288,0,1,2,0};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleCameraFollowProfile profile;check(profile.load({bytes.data(),bytes.size()},&error),"source profile");
    BattleCameraFollowInput input;input.actor.actor_position={100,20,300};input.actor.values88c[0]=180;input.actor.values88c[2]=500;input.actor.values88c[3]=600;input.actor.scale_b4=1;input.width668=40;
    BattleCameraFollowState before;before.target={100,200,300};before.horizontal318={100,200,-2100};before.eye={100,552,-2074};before.distance348=2400;before.height350=180;before.scale354=1;before.angles70={1536,0,0};
    BattleCameraFollowPlan plan;check(battle_camera_first_follow(profile,input,before,&plan,&error),error.c_str());
    check(plan.state.distance348==600&&plan.minimum==500&&plan.maximum==600&&plan.radius_clamped&&!plan.pitch_clamped&&plan.previous_eye==before.eye&&plan.previous_target==before.target,"history and radius clamp follow source order");
    check(std::abs(plan.state.horizontal318[2]+1380)<.002f&&plan.state.angles70[0]>0&&plan.state.angles70[0]<100,"source4/5 then3/4 horizontal smoothing, quantized pitch");
    check(plan.height_probe_eye!=plan.state.eye&&plan.pre_clamp_distance>plan.maximum,"floor query uses pre-FFFC eye");
    const auto parameters=stage_ground_fixture::parameters();const auto middle=f32(f32(plan.height_probe_eye[1]+plan.state.eye[1])*.5f);
    auto data=stage_ground_fixture::bytes({stage_ground_fixture::triangle(middle)});StihGrid floor;check(floor.load({data.data(),data.size()},&error),"owned floor grid");
    StageLowestFloorResult a,b;check(stage_lowest_floor(floor,parameters,plan.height_probe_eye,&a,&error)&&stage_lowest_floor(floor,parameters,plan.state.eye,&b,&error)&&a.found&&b.found,"source floor queries");
    check((a.height>plan.height_probe_eye[1])!=(b.height>plan.state.eye[1]),"substituting final eye would change the actual source branch");
    data=stage_ground_fixture::bytes({stage_ground_fixture::triangle(70),stage_ground_fixture::triangle(-20),stage_ground_fixture::triangle(10)});check(floor.load({data.data(),data.size()},&error)&&stage_lowest_floor(floor,parameters,{-100,50,-100},&a,&error)&&a.height==-20&&a.triangle==1&&a.visited==3,"3DD34 selects minimum floor, not nearest or highest");
    data=stage_ground_fixture::bytes({stage_ground_fixture::triangle(5000)});check(floor.load({data.data(),data.size()},&error)&&stage_lowest_floor(floor,parameters,{-100,50,-100},&a,&error)&&a.found&&a.height==parameters.low68,"source high34 equality uses low68 sentinel fallback");
    StihGrid empty;check(stage_lowest_floor(empty,parameters,{7500,123,0},&a,&error)&&!a.inside&&!a.visited&&a.height==123,"strict source global-bound early return needs no grid read");
    input.actor.actor_position[1]=10000;input.width668=1e6f;check(battle_camera_first_follow(profile,input,before,&plan,&error)&&plan.pitch_clamped&&std::abs(int(plan.state.angles70[0]))<=3686,"source pitch clamp branch uses target distance and width");
    input.target508_bound=true;check(!battle_camera_first_follow(profile,input,before,&plan,&error),"targeted camera mode remains unsupported");input.target508_bound=false;
    input.lock7c9=1;check(!battle_camera_first_follow(profile,input,before,&plan,&error),"special lock mode remains unsupported");input.lock7c9=0;
    before.extended35e=1;check(!battle_camera_first_follow(profile,input,before,&plan,&error),"later extended-distance state cannot masquerade as first follow");
    std::printf("battle camera first follow: %u checks PASS\n",checks);
}
