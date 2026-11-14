#pragma once
// Source3FB00/3FA68/3F320/3D6E4 static-grid query. Not collision response,
// moving-platform ownership or all-stage/battle admission. Explicit actor fields.
#include "engine/assets/stih.hpp"
namespace gf {
struct StageGroundActor {
    bool fields_declared=false;u8 attached1db=0;f32 current24=0,previous30=0,offset6d0=0,previous6d4=0;
};
namespace stage_ground_detail {
inline bool number(f32 v){return std::isfinite(v)&&std::abs(v)<=1e7f;}
inline bool vector(const std::array<f32,3>& v){for(auto x:v)if(!number(x))return false;return true;}
inline bool parameters(const StageGroundParameters& p){return stage_ground_parameters_valid(p);}
inline bool inside(const StageGroundParameters& p,const std::array<f32,3>& v){return p.bounds_min<v[0]&&v[0]<p.bounds_max&&p.bounds_min<v[2]&&v[2]<p.bounds_max;}
inline bool cell(const StihGrid& g,const StageGroundParameters& p,const std::array<f32,3>& v,std::span<const u32>* refs){
    if(!g.loaded()||f32(g.cell_size_x())!=p.cell_size||f32(g.cell_size_z())!=p.cell_size)return false;
    const f32 x=f32(f32(v[0]-f32(g.minimum_x()))/p.cell_size),z=f32(f32(v[2]-f32(g.minimum_z()))/p.cell_size);
    if(!number(x)||!number(z)||x<=-1||z<=-1||x>=f32(g.cells_x())||z>=f32(g.cells_z()))return false;
    // Source fctiwz truncates toward zero, including negative fractions.
    const s32 ix=s32(x),iz=s32(z);if(ix<0||iz<0)return false;*refs=g.cell(unsigned(ix),unsigned(iz));return true;
}
inline bool projected(const StihTriangle& t,const std::array<f32,3>& point,bool ceiling,std::array<f32,2>* delta){
    std::array<f32,3> x,z;for(unsigned i=0;i<3;++i){x[i]=f32(t.vertices[i][0]-point[0]);z[i]=f32(t.vertices[i][2]-point[2]);}
    for(unsigned i=0;i<3;++i){const unsigned j=(i+1)%3;const f32 a=f32(z[i]*x[j]),b=f32(x[i]*z[j]);
        if(ceiling){if(f32(a-b)>0||a>b)return false;}else if(a<b)return false;
    }*delta={x[0],z[0]};return true;
}
inline bool height(const StihTriangle& t,const std::array<f32,2>& delta,f32* out){
    if(t.plane[1]==0)return false;
    f32 y=f32(t.plane[0]*delta[0]);y=f32(y+f32(t.plane[2]*delta[1]));y=f32(y+f32(t.plane[1]*t.vertices[0][1]));y=f32(y/t.plane[1]);
    if(!number(y))return false;
    *out=y;return true;
}
inline bool ceiling(const std::array<const StihGrid*,3>& grids,const StageGroundParameters& p,const std::array<f32,3>& point,f32 above,f32* out){
    f32 best=p.high34;for(unsigned slot:{2u,1u}){if(!grids[slot])return false;std::span<const u32> refs;if(!cell(*grids[slot],p,point,&refs))return false;
        for(u32 index:refs){const auto& t=grids[slot]->triangles()[index];std::array<f32,2> delta;if(!projected(t,point,true,&delta))continue;
            if(slot==1&&t.plane[1]>p.secondary_normal_limit)continue;
            f32 y=0;if(!height(t,delta,&y))return false;if(above<y&&best>=y)best=y;
        }
    }*out=best;return true;
}
}
inline bool stage_ground_height(const std::array<const StihGrid*,3>& grids,const StageGroundParameters& p,const StageGroundActor& actor,
                                const std::array<f32,3>& position,f32 height,f32* out,std::string* error){
    if(out)*out=0;
    auto fail=[&](const char* why){if(error)*error=std::string("stage ground: ")+why;return false;};
    if(!out||!actor.fields_declared||actor.attached1db)return fail("declared static actor fields; moving-platform branch not implemented");
    using namespace stage_ground_detail;
    if(!parameters(p)||!vector(position)||!number(height))return fail("parameter/position/height domain");
    if(!inside(p,position)){*out=position[1];if(error)error->clear();return true;}
    for(f32 v:{actor.current24,actor.previous30,actor.offset6d0,actor.previous6d4})if(!number(v))return fail("actor scalar domain");
    const bool raised=height>=p.high34;auto point=position;point[1]=f32(position[1]+actor.offset6d0);
    const f32 previous=f32(actor.previous30+actor.previous6d4),current=f32(actor.current24+actor.offset6d0),span=f32(previous-current);
    if(!vector(point)||!number(previous)||!number(current)||!number(span))return fail("derived actor domain");
    f32 ceiling=0;if(!stage_ground_detail::ceiling(grids,p,point,previous,&ceiling))return fail("ceiling grid/query domain");
    if(span>p.zero)height=f32(height+span);
    height=f32(height+position[1]);if(!number(height))return fail("height span domain");
    if(!grids[0])return fail("floor grid required");
    std::span<const u32> refs;if(!cell(*grids[0],p,position,&refs))return fail("floor cell domain");
    f32 best=p.low68,low=p.high34,high=p.low68;
    for(u32 index:refs){const auto& t=grids[0]->triangles()[index];std::array<f32,2> delta;if(!projected(t,point,false,&delta))continue;
        f32 y=0;if(!stage_ground_detail::height(t,delta,&y))return fail("floor normal/height domain");
        if(height>y&&ceiling>y&&best<=y)best=y;
        if(low>=y)low=y;
        if(high<=y)high=y;
    }
    *out=raised?high:best!=p.low68?best:low==p.high34?p.low68:low;if(error)error->clear();return true;
}
} // namespace gf
