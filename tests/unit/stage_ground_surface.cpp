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
    auto adjacent=a;
    adjacent.vertices={a.vertices[1],{1000,260,1000},a.vertices[2]};
    auto crease=adjacent;crease.plane[0]+=.00001f;
    auto gap=adjacent;gap.vertices[0][2]+=1;
    auto overlap=adjacent;overlap.vertices[1]={-500,-115,-500};
    load(floor,{a,adjacent,crease,gap,overlap,a});
    const std::array<f32,3> a_point{-100,-15,0},b_point{100,35,0};
    check(stage_ground_coplanar_crossing(floor,0,1,a_point,b_point)&&stage_ground_coplanar_crossing(floor,1,0,b_point,a_point));
    for(u32 index:{0u,2u,3u,4u,5u,6u})check(!stage_ground_coplanar_crossing(floor,0,index,a_point,b_point));
    auto narrow_left=a,narrow_right=a;
    narrow_left.vertices={{{0,10,0},{0,10,10},{-10,7.5f,30}}};
    narrow_right.vertices={{{0,10,10},{0,10,0},{10,12.5f,30}}};
    load(floor,{narrow_left,narrow_right});
    check(stage_ground_coplanar_crossing(floor,0,1,{-1,9.75f,5},{1,10.25f,5}));
    check(!stage_ground_coplanar_crossing(floor,0,1,{-6,8.5f,20},{6,11.5f,20}));
    auto gentle=adjacent;gentle.plane={-.27f,1,-.02f,-10};gentle.vertices[1][1]=300;
    auto sharp=adjacent;sharp.plane={-.65f,1,-.4f,-10};sharp.vertices[1][1]=1060;
    // Exact shared vertices alone do not make independently stored planes
    // agree when the height evaluator is anchored at vertex zero.
    auto discontinuous=gentle;discontinuous.plane[2]=0.f;
    load(floor,{a,gentle,sharp,discontinuous,gap,overlap});
    check(!stage_ground_coplanar_crossing(floor,0,1,a_point,b_point));
    check(stage_ground_slope_crossing(floor,0,1,a_point,b_point,true));
    for(u32 index:{2u,3u,4u,5u})check(!stage_ground_slope_crossing(floor,0,index,a_point,b_point,true));
    auto face=a;
    face.vertices={{{100,20,100},{100,20,120},{120,40,100}}};face.plane={-1,1,0,80};
    auto low=face;for(auto& v:low.vertices)v[1]=0;low.plane={0,1,0,0};
    const std::array<f32,3> start{104,24,106},approach{106,20,106},exit{107,27,120};
    load(floor,{face,low});
    check(stage_ground_same_face_push_exit(floor,p,0,start,approach,exit));
    // Neither late entry from below nor starting inside the face below its plane.
    check(!stage_ground_same_face_push_exit(floor,p,0,{99,19,106},{106,16,106},exit));
    check(!stage_ground_same_face_push_exit(floor,p,0,{104,23.99f,106},approach,exit));
    check(!stage_ground_same_face_push_exit(floor,p,0,{104,24.01f,106},approach,exit));
    check(!stage_ground_same_face_push_exit(floor,p,0,start,{106,26,106},exit)); //no incoming crossing
    check(!stage_ground_same_face_push_exit(floor,p,0,start,approach,{107,26.95f,120})); //subsurface projected travel
    check(!stage_ground_same_face_push_exit(floor,p,0,start,approach,{106,26,106})); //no edge exit
    check(!stage_ground_same_face_push_exit(floor,p,0,start,approach,{96,23,94})); //exact corner exit
    check(!stage_ground_same_face_push_exit(floor,p,0,start,approach,{540,460,106})); //unreviewed cell
    check(!stage_ground_same_face_push_exit(floor,p,2,start,approach,exit)); //not a live face in the cell
    auto hidden=low;hidden.vertices={{{105,31,111},{105,31,115},{107,31,111}}};hidden.plane={0,1,0,-31};
    load(floor,{face,low,hidden});
    check(!stage_ground_same_face_push_exit(floor,p,0,start,approach,exit)); //competing face under the segment only
    using A=StageFloorApproach;
    StageFloorApproachTrace trace;
    auto classification=[&](const std::array<f32,3>& from,const std::array<f32,3>& to,A kind){
        check(stage_ground_trace_floor_approach(floor,0,from,to,&trace)&&trace.kind==kind);
    };
    classification(start,approach,A::DescendsFromFace);
    check(trace.enter==0&&trace.exit==1&&trace.clearance_at_entry==0&&trace.clearance_at_exit==-6);
    classification({99,19,106},{106,16,106},A::EntersBelow);
    check(trace.enter>0&&trace.exit==1&&trace.clearance_at_entry< -1&&trace.clearance_at_exit< -9);
    classification({104,26,106},approach,A::CrossesFromAbove);
    classification({104,23,106},approach,A::StartsBelow);
    classification({104,60,106},{106,59,106},A::ClearAbove);
    classification({99,20,106},{98,20,106},A::MissesFootprint);
    classification({95,25,105},{105,25,95},A::BoundaryOnly); //only the vertex is shared
    check(trace.enter==.5&&trace.exit==.5);
    classification({100,24,105},{100,24,115},A::BoundaryOnly); //along an edge, not through the interior
    classification(start,exit,A::NearPlane);
    check(!stage_ground_trace_floor_approach(floor,3,start,approach,&trace)&&
        !stage_ground_trace_floor_approach(floor,0,{1e8f,0,0},approach,&trace));
    std::printf("static surface selection, slope seams, projection exit and approach diagnostics: %u checks PASS\n",checks);
}
