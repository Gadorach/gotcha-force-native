#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/stage_inclined_ledge.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Inclined ledge: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using Point=std::array<f32,3>;
    const std::array<StihTriangle,3> strip{{
        {0,{{{0,100,-2},{0,100,2},{-3,101.5f,0}}},{.4472136f,.8944272f,0,0}},
        {0,{{{0,100,2},{0,100,-2},{2,94,1}}},{.9486833f,.3162278f,0,0}},
        {0,{{{-10,0,-8},{12,0,-8},{0,0,15}}},{0,1,0,0}},
    }};
    const Point a{-1,100.5f,0},b{5,100.5f,0};
    check(stage_inclined_ledge_crossing(strip,0,2,a,b,60),"interior departure from downhill face over an exact steep connector");
    check(!stage_inclined_ledge_crossing(strip,0,2,a,b,97),"the lip of the drop must clear the step limit, not just the endpoints");
    check(!stage_inclined_ledge_crossing(strip,0,2,b,a,60),"reverse travel cannot use the descending witness");
    auto wrong=b;wrong[1]=0;
    check(!stage_inclined_ledge_crossing(strip,0,2,a,wrong,60),"instant snapping down to the flat floor is not a departure");
    auto vertex=strip;vertex[1].vertices[2][2]=0;
    check(!stage_inclined_ledge_crossing(vertex,0,2,a,b,60),"exiting exactly through a cliff vertex refuses");
    auto gap=strip;
    for(auto& v:gap[1].vertices)v[0]+=.05f;
    check(!stage_inclined_ledge_crossing(gap,0,2,a,b,60),"an almost shared edge cannot bridge a gap");
    auto mismatch=strip;mismatch[1].vertices[2][1]+=.1f;
    check(!stage_inclined_ledge_crossing(mismatch,0,2,a,b,60),"mismatched cliff plane and authored vertices refuse");
    auto shallow=strip;shallow[1].vertices[2][1]=99;shallow[1].plane=strip[0].plane;
    check(!stage_inclined_ledge_crossing(shallow,0,2,a,b,60),"shallow continuation is not a cliff");
    std::vector<StihTriangle> obstructed(strip.begin(),strip.end());
    obstructed.push_back({0,{{{2,65,-1},{4,65,-1},{3,65,2}}},{0,1,0,0}});
    check(!stage_inclined_ledge_crossing(obstructed,0,2,a,b,60),"intermediate elevated support hidden from endpoint queries refuses");
    for(auto& v:obstructed.back().vertices)v[1]=-1;
    check(stage_inclined_ledge_crossing(obstructed,0,2,a,b,60),"completely buried face cannot interrupt the witness");
    std::puts("Inclined ledge: exact steep strip, interior exit and intermediate obstruction checks PASS");
}
