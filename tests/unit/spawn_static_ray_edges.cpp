#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/challenge_spawn_static_tail.hpp"
#include "tests/unit/challenge_battle_spawn_fixture.hpp"
namespace {unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"static ray edges: %s\n",why);std::abort();}}
std::vector<gf::u8> grid(){
    using namespace gf;using challenge_battle_spawn_fixture::p32;
    //2x2 cells all refer to the SAME triangles: enumeration must not deduplicate.
    std::vector<u8> b(216);p32(b,0,0x53544948);p32(b,4,u32(b.size()));p32(b,8,500);p32(b,12,500);p32(b,16,2);p32(b,20,2);p32(b,32,32);p32(b,36,96);
    for(unsigned i=0;i<4;++i){p32(b,40+4*i,48+12*i);p32(b,56+12*i,0);p32(b,60+12*i,56);p32(b,64+12*i,0xffffffffu);}
    for(unsigned i=0;i<2;++i){const unsigned at=104+56*i;const f32 y=i?20:10;const f32 values[13]={0,y,0,0,y,1000,1000,y,0,0,1,0,-y};for(unsigned k=0;k<13;++k)p32(b,at+4+4*k,f32_to_bits(values[k]));}return b;
}}
int main(){
    using namespace gf;using Vec=std::array<f32,3>;SourceSegmentPlaneHit hit;const f32 tiny=std::numeric_limits<f32>::denorm_min();
    //Subnormal distances still take the ordinary ratio branch; no epsilon.
    check(source_segment_plane({1,0,0},{-1,0,0},{tiny,0,0},{0,0,0},&hit)&&hit.side==1&&hit.point==Vec{0,0,0},"subnormal positive denominator retains ordinary ratio branch");
    const f32 huge=1e7f;check(source_segment_plane({huge,0,0},{huge,1,0},{1,0,0},{huge,0,0},&hit)&&!hit.side,"parallel zero plane distances no hit");
    bool inside=true;const std::array<Vec,3> triangle{{{0,0,0},{0,0,100},{100,0,0}}};check(source_triangle_contains({20,0,20},triangle,{0,-1,0},&inside)&&!inside,"opposite normal reverses containment");check(source_triangle_contains({0,0,0},triangle,{0,0,0},&inside)&&inside,"source degenerate zero-normal inequalities, not a new rejection policy");
    check(!source_segment_plane({NAN,0,0},{0,0,0},{0,1,0},{0,0,0},&hit)&&hit==SourceSegmentPlaneHit{},"invalid input clears plane candidate");check(!source_triangle_contains({0,0,0},triangle,{NAN,0,0},&inside)&&!inside,"invalid normal clears containment");
    auto b=grid();StihGrid data;std::string error;check(data.load({b.data(),b.size()},&error),"owned repeated-cell fixture");StageRayParameters p{0,999,0,999,500,true};StageSegmentHit result;
    check(stage_segment_query(data,p,{50,30,50},{550,0,550},&result,&error)&&result.hit&&result.triangle==1&&result.cell==0&&result.tested==8,"inclusive rectangle/no dedup/tie keeps first encountered cell");
    check(stage_segment_query(data,p,{0,30,0},{0,0,0},&result,&error)&&result.hit&&result.tested==2,"minimum scene bounds and triangle vertex inclusive");
    p.max_x=p.max_z=500;check(stage_segment_query(data,p,{500,30,0},{500,0,0},&result,&error)&&result.hit&&result.cell==2,"maximum scene bound inclusive with valid next cell");
    p.declared=false;check(!stage_segment_query(data,p,{50,30,50},{50,0,50},&result,&error)&&result==StageSegmentHit{},"undeclared scene bounds refuse before no-hit");p.declared=true;p.cell_size=499;check(!stage_segment_query(data,p,{50,30,50},{50,0,50},&result,&error)&&result==StageSegmentHit{},"source fixed cell operand required");
    p.cell_size=500;auto bad=b;bad[0]=0;check(!data.load({bad.data(),bad.size()},&error),"failed reload invalidates grid");
    check(!stage_segment_query(data,p,{50,30,50},{50,0,50},&result,&error)&&result==StageSegmentHit{},"unavailable grid cannot produce a no-hit receipt or replay an old hit");
    s16 angle=123;check(source_heading_s16(0,0,&angle)&&angle==0,"explicit zero vector host atan2 model");check(source_heading_s16(-0.f,-0.f,&angle)&&angle==-32768,"negative-zero pair preserves atan2 quadrant");check(!source_heading_s16(1e8f,0,&angle)&&angle==0,"heading domain no stale angle");
    std::printf("static ray edges: %u owned checks PASS\n",checks);
}
