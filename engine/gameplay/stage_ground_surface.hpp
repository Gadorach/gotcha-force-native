#pragma once
#include "engine/gameplay/stage_ground.hpp"
namespace gf {
struct StageGroundSurfaceResult {
    f32 height=0;std::array<f32,3> normal{0,1,0};u32 visited=0,triangle=0;
    bool inside=false,selected=false;
    bool operator==(const StageGroundSurfaceResult&)const=default;
};
//3E0A8's STATIC height+normal sibling. Reuses shared cell/projection/plane and
//ceiling kernels; its outside return is the source constant-5000, NOT pointY.
//Moving platforms and3F688's nonzero87/capsule-query branch remain refused.
inline bool stage_ground_surface(const std::array<const StihGrid*,3>& grids,const StageGroundParameters& p,const StageGroundActor& actor,const std::array<f32,3>& position,f32 height,StageGroundSurfaceResult* out,std::string* error){
    auto fail=[&](){if(error)*error="stage surface: current static source grids/actor/plane required";return false;};
    using namespace stage_ground_detail;
    if(!out||!actor.fields_declared||actor.attached1db||!parameters(p)||!vector(position)||!number(height))return fail();
    StageGroundSurfaceResult result;result.height=-5000;
    if(!inside(p,position)){*out=result;if(error)error->clear();return true;}
    result.inside=true;for(f32 v:{actor.current24,actor.previous30,actor.offset6d0,actor.previous6d4})if(!number(v))return fail();
    const bool raised=height>=p.high34;auto point=position;point[1]=f32(position[1]+actor.offset6d0);
    const f32 previous=f32(actor.previous30+actor.previous6d4),current=f32(actor.current24+actor.offset6d0),span=f32(previous-current);
    if(!vector(point)||!number(previous)||!number(current)||!number(span))return fail();
    f32 ceiling=0;if(!stage_ground_detail::ceiling(grids,p,point,previous,&ceiling))return fail();
    if(span>p.zero)height=f32(height+span);
    height=f32(height+position[1]);if(!number(height)||!grids[0])return fail();
    std::span<const u32> refs;if(!cell(*grids[0],p,position,&refs))return fail();
    f32 best=p.low68,low=p.high34,high=p.low68;std::array<f32,3> best_n{0,1,0},low_n{0,1,0},high_n{0,1,0};u32 best_i=0,low_i=0,high_i=0;bool best_set=false,low_set=false,high_set=false;
    for(u32 index:refs){
        ++result.visited;const auto& t=grids[0]->triangles()[index];std::array<f32,2> delta;if(!projected(t,point,false,&delta))continue;
        f32 y=0;if(!stage_ground_detail::height(t,delta,&y))return fail();
        const std::array<f32,3> normal{t.plane[0],t.plane[1],t.plane[2]};if(!vector(normal))return fail();
        if(height>y&&ceiling>y&&best<=y){best=y;best_n=normal;best_i=index;best_set=true;}
        if(low>=y){low=y;low_n=normal;low_i=index;low_set=true;}
        if(high<=y){high=y;high_n=normal;high_i=index;high_set=true;}
    }
    if(raised){result.height=high;result.normal=high_n;result.triangle=high_i;result.selected=high_set;}
    else if(best!=p.low68){result.height=best;result.normal=best_n;result.triangle=best_i;result.selected=best_set;}
    else{result.normal=low_n;result.triangle=low_i;result.selected=low_set;result.height=low==p.high34?p.low68:low;}
    *out=result;if(error){error->clear();}return true;
}
}
