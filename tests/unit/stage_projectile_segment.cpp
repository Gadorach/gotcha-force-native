#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/stage_projectile_segment.hpp"
#include "tests/unit/challenge_battle_spawn_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"stage projectile segment: %s\n",why);std::abort();}
}
std::vector<gf::u8> grid(gf::f32 y){
    using namespace gf;
    using challenge_battle_spawn_fixture::p32;
    std::vector<u8> b(108);
    p32(b,0,0x53544948);p32(b,4,u32(b.size()));p32(b,8,500);p32(b,12,500);p32(b,16,1);p32(b,20,1);
    p32(b,32,32);p32(b,36,44);p32(b,40,36);p32(b,44,0);p32(b,48,0xffffffffu);
    const f32 fields[]={0,y,0,0,y,500,500,y,0,0,1,0,-y};
    for(unsigned i=0;i<13;++i)p32(b,56+i*4,f32_to_bits(fields[i]));
    return b;
}
}
int main(){
    using namespace gf;
    using V=std::array<f32,3>;
    std::string error;
    std::array<StihGrid,3> inputs;
    for(unsigned i=0;i<inputs.size();++i){const auto b=grid(i?20.f:10.f);check(inputs[i].load({b.data(),b.size()},&error),"three owned layer fixtures");}
    std::array<const StihGrid*,3> grids{&inputs[0],&inputs[1],&inputs[2]};
    constexpr StageRayParameters p{0,499,0,499,500,true};
    StageProjectileSegment out;
    check(stage_projectile_segment(grids,p,{30,0,30},{30,30,30},&out,&error)&&
        out.queried&&out.surface.grid_code==3&&out.position==V{30,20,30}&&out.surface.nearest.tested==3,
        "scan near previous first; tertiary wins a tie");
    check(stage_projectile_segment(grids,p,{30,30,30},{30,0,30},&out,&error)&&
        out.surface.grid_code==1&&out.position==V{30,10,30}&&out.surface.nearest.tested==3,
        "reversed segment keeps the primary surface nearer its new first endpoint");
    check(stage_projectile_segment(grids,p,{-100,30,-100},{100,0,100},&out,&error)&&
        out.clipped_current==V{0,15,0}&&out.clipped_previous==V{100,0,100}&&out.surface.grid_code==1,
        "endpoint clip adjusts height along the segment");
    check(stage_projectile_segment(grids,p,{-100,60,-200},{100,0,100},&out,&error)&&
        out.clipped_current==V{f32(100.f-200.f*100.f/300.f),20,0},
        "Z crossing follows the earlier X correction");
    check(stage_projectile_segment(grids,p,{30,70,30},{60,50,60},&out,&error)&&
        out.queried&&!out.surface.grid_code&&out.surface.nearest.tested==3&&out.position==V{30,70,30},
        "computed miss preserves the unmodified current position");
    check(stage_projectile_segment(grids,p,{30,30,30},{30,30.0005f,30},&out,&error)&&
        !out.queried&&!out.surface.grid_code&&out.position==V{30,30,30},"sub-threshold separation bypasses all triangles");
    check(!stage_projectile_segment(grids,p,{-100,30,30},{-100,0,30},&out,&error)&&out.position==V{30,30,30},
        "degenerate clip refuses without publishing a candidate");
    grids[1]=nullptr;
    check(!stage_projectile_segment(grids,p,{30,30,30},{30,30,30},&out,&error)&&!out.queried,
        "missing grid cannot masquerade as a trivial miss");
    std::puts("stage projectile segment: detached endpoint clips and ordered grid hits PASS");
}
