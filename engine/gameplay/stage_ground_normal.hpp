#pragma once
#include "engine/gameplay/stage_ground.hpp"
namespace gf {
struct StageGroundNormal {std::array<f32,3> normal{0,1,0};bool inside=false,selected=false;f32 selected_height=0;};
//3DE80 static branch: source default UP is written even for outside/no surface.
//This is not a height-query fallback, moving-platform normal or contact ACK.
inline bool stage_ground_surface_normal(const std::array<const StihGrid*,3>& grids,const StageGroundParameters& p,const StageGroundActor& actor,const std::array<f32,3>& position,f32 height,StageGroundNormal* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("stage ground normal: ")+why;return false;};
    using namespace stage_ground_detail;
    if(!out||!actor.fields_declared||actor.attached1db)return fail("declared static fields required; attached normal unsupported");
    if(!parameters(p)||!vector(position)||!number(height))return fail("source parameter/position/height domain");
    StageGroundNormal next;if(!inside(p,position)){*out=next;if(error)error->clear();return true;}
    for(f32 v:{actor.current24,actor.previous30,actor.offset6d0,actor.previous6d4})if(!number(v))return fail("source history domain");
    next.inside=true;auto point=position;point[1]=f32(point[1]+actor.offset6d0);const f32 previous=f32(actor.previous30+actor.previous6d4),current=f32(actor.current24+actor.offset6d0),span=f32(previous-current);if(!vector(point)||!number(previous)||!number(current)||!number(span))return fail("derived history domain");
    f32 ceiling=0;if(!stage_ground_detail::ceiling(grids,p,point,previous,&ceiling))return fail("ceiling query domain");
    if(span>0)height=f32(height+span);
    height=f32(height+position[1]);if(!number(height)||!grids[0])return fail("floor height/grid domain");
    std::span<const u32> refs;if(!cell(*grids[0],p,position,&refs))return fail("floor cell domain");
    f32 best=p.low68;for(u32 index:refs){const auto& t=grids[0]->triangles()[index];std::array<f32,2> delta;if(!projected(t,point,false,&delta))continue;f32 y=0;if(!stage_ground_detail::height(t,delta,&y))return fail("floor normal/height domain");if(height>y&&ceiling>y&&best<=y){next.normal={t.plane[0],t.plane[1],t.plane[2]};next.selected_height=y;next.selected=true;best=y;}}
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
