#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/stage_ground_surface.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const auto p=stage_ground_fixture::parameters();StageGroundActor actor{true,0,50,50,0,0};
    auto load=[&](StihGrid& grid,const std::vector<StihTriangle>& triangles){const auto raw=stage_ground_fixture::bytes(triangles);check(grid.load({raw.data(),raw.size()},&error));};
    auto tilted=[](f32 y,f32 slope){auto t=stage_ground_fixture::triangle(y);for(auto& v:t.vertices)v[1]=f32(y+slope*v[0]);t.plane={-slope,1,0,-y};return t;};
    StihGrid floor,empty,ceil;load(empty,{});load(ceil,{stage_ground_fixture::triangle(1000,true)});
    const std::array<const StihGrid*,3> grids{&floor,&empty,&ceil};const std::array<f32,3> point{-10,50,-10};StageGroundSurfaceResult result;
    //Different normals at exactly tied heights: every selected branch is last-wins.
    const auto a=tilted(10,.25f),b=tilted(12.5f,.5f);load(floor,{a,b});
    check(stage_ground_surface(grids,p,actor,point,30,&result,&error)&&result.selected&&result.triangle==1&&result.height==7.5f&&result.normal==std::array<f32,3>{-.5f,1,0});
    check(stage_ground_surface(grids,p,actor,point,p.high34,&result,&error)&&result.triangle==1&&result.height==7.5f&&result.normal==std::array<f32,3>{-.5f,1,0});
    check(stage_ground_surface(grids,p,actor,point,-100,&result,&error)&&result.triangle==1&&result.height==7.5f&&result.normal==std::array<f32,3>{-.5f,1,0});
    //Strict below-height and below-ceiling gates, then lowest-floor fallback.
    load(floor,{stage_ground_fixture::triangle(0),tilted(80,1)});
    check(stage_ground_surface(grids,p,actor,point,20,&result,&error)&&result.triangle==0&&result.height==0&&result.normal==std::array<f32,3>{0,1,0});
    check(stage_ground_surface(grids,p,actor,point,21,&result,&error)&&result.triangle==1&&result.height==70&&result.normal==std::array<f32,3>{-1,1,0});
    load(ceil,{stage_ground_fixture::triangle(70,true)});
    check(stage_ground_surface(grids,p,actor,point,30,&result,&error)&&result.triangle==0&&result.height==0);
    //Actual previous/current height span changes the height gate, not the query XZ.
    load(ceil,{stage_ground_fixture::triangle(1000,true)});auto descending=actor;descending.previous30=51;
    check(stage_ground_surface(grids,p,descending,point,20,&result,&error)&&result.triangle==1&&result.height==70);
    load(floor,{});
    for(f32 height:{30.f,p.high34})check(stage_ground_surface(grids,p,actor,point,height,&result,&error)&&!result.selected&&result.height==p.low68&&result.normal==std::array<f32,3>{0,1,0});
    for(const std::array<f32,3> outside:{std::array<f32,3>{p.bounds_min,50,0},{p.bounds_max,50,0},{0,50,p.bounds_min},{0,50,p.bounds_max}})check(stage_ground_surface(grids,p,actor,outside,30,&result,&error)&&!result.inside&&!result.selected&&result.height==-5000&&result.normal==std::array<f32,3>{0,1,0});
    auto attached=actor;attached.attached1db=1;check(!stage_ground_surface(grids,p,attached,point,30,&result,&error));
    auto undeclared=actor;undeclared.fields_declared=false;check(!stage_ground_surface(grids,p,undeclared,point,30,&result,&error));
    std::printf("static surface selected normals, ties, strict gates, span, empty and bounds: %u checks PASS\n",checks);
}
