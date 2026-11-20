#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/stage_ground_post_solid_floor.hpp"
#include "tests/unit/stage_ground_fixture.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;if(!ok){std::fprintf(stderr,"post-solid floor: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    StihGrid floor,empty,ceiling;std::string error;
    auto load=[&](StihGrid& grid,const std::vector<StihTriangle>& triangles){
        const auto data=stage_ground_fixture::bytes(triangles);
        check(grid.load({data.data(),data.size()},&error),"synthetic original-format grid");
    };
    load(floor,{stage_ground_fixture::triangle(10)});load(empty,{});load(ceiling,{stage_ground_fixture::triangle(1000,true)});
    const std::array<const StihGrid*,3> grids{&floor,&empty,&ceiling};
    const auto p=stage_ground_fixture::parameters();
    StagePostSolidFloorInput in;
    in.actor={true,0,4,4,2,2};in.position={-10,4,-10};in.solid_displacement={3,-4,0};in.query_height=60;
    in.occupied=true;in.hold1df=7;
    StagePostSolidFloorResult out;
    check(stage_ground_post_solid_floor({},p,in,&out,&error)&&!out.queried&&out.position==in.position&&out.hold1df==7,
          "no solid movement means no floor lookup or implicit snap");
    in.moved1dd=0x80;in.occupied=false;
    check(stage_ground_post_solid_floor({},p,in,&out,&error)&&!out.queried&&out.position==in.position&&out.hold1df==7,
          "an unused actor row cannot query from a stale movement bit");
    in.occupied=true;
    auto correct=[&](f32 y,f32 floor_y,f32 lift,u8 hold){
        check(stage_ground_post_solid_floor(grids,p,in,&out,&error)&&out.queried&&out.queried_height==floor_y&&out.lift==lift&&
              out.position==std::array<f32,3>{in.position[0],y,in.position[2]}&&out.hold1df==hold,
              "height-only lift, unchanged planar position, and exact recovery predicate");
    };
    correct(8,10,4,2); // nonzero high bit is sufficient; retain the origin's +2 floor offset
    in.moved1dd=1;
    in.solid_displacement={4,-1,0};correct(8,10,4,7);
    in.solid_displacement={0,0,0};correct(8,10,4,7);
    in.solid_displacement={0,4,0};correct(8,10,4,7);
    in.solid_displacement={1.73205078f,-1,0};correct(8,10,4,2); // host-normalized Y rounds to -0.5
    in.solid_displacement={1.732052f,-1,0};correct(8,10,4,7);
    in.solid_displacement={3,-4,0};in.position[1]=in.actor.current24=8;
    correct(8,10,0,7); // equality does not refresh the hold, even for a steep downward response
    in.position[1]=in.actor.current24=15;
    correct(15,10,0,7); // never drags down to the floor
    in.position[1]=in.actor.current24=4;
    load(floor,{stage_ground_fixture::triangle(10),stage_ground_fixture::triangle(40)});
    correct(38,40,34,2); // use the shared declared-height query, not a platform impulse
    in.query_height=4;
    correct(8,10,4,2); // original low-floor fallback when no floor passes the height cutoff
    const auto retained=out;
    auto refuses=[&](const StagePostSolidFloorInput& bad,const auto& stage){
        check(!stage_ground_post_solid_floor(stage,p,bad,&out,&error)&&out==retained,"unsupported input cannot publish any correction");
    };
    auto bad=in;bad.actor.attached1db=1;refuses(bad,grids);
    bad=in;bad.actor.current24=5;refuses(bad,grids);
    bad=in;bad.actor.fields_declared=false;refuses(bad,grids);
    bad=in;bad.solid_displacement[0]=1e8f;refuses(bad,grids);
    bad=in;bad.position[0]=p.bounds_max;refuses(bad,grids);
    refuses(in,std::array<const StihGrid*,3>{});
    std::printf("detached post-solid floor correction: %u checks PASS\n",checks);
}
