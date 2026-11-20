#pragma once
#include "engine/gameplay/stage_segment_query.hpp"

namespace gf {
struct StageProjectileSegment {
    std::array<f32,3> clipped_current{},clipped_previous{},position{};
    StageAllSegmentHit surface;
    bool queried=false;
};
namespace stage_projectile_segment_detail {
using source_intersection_detail::Vec;
// 84230 adjusts X then Z relative to the OTHER unmodified endpoint.
// Degenerate divisions refuse rather than silently clamp in each axis.
inline bool clip(Vec v,const Vec& other,const StageRayParameters& p,Vec* out){
    const auto axis=[&](unsigned i,unsigned cross,f32 lo,f32 hi){
        if(v[i]>=lo&&v[i]<=hi)return true;
        const f32 edge=v[i]<lo?lo:hi;
        const f32 span=f32(v[i]-other[i]);if(!span)return false;
        const f32 offset=f32(edge-other[i]);
        for(unsigned j:{cross,1u})v[j]=f32(other[j]+f32(f32(offset*f32(v[j]-other[j]))/span));
        v[i]=edge;
        return source_intersection_detail::valid(v);
    };
    if(!axis(0,2,p.min_x,p.max_x)||!axis(2,0,p.min_z,p.max_z))return false;
    *out=v;return true;
}
}
// Detached 84170 geometry slice. A hit supplies a candidate position only,
// not a live stage-effect call, an actor collision, or a retirement receipt.
inline bool stage_projectile_segment(const std::array<const StihGrid*,3>& grids,const StageRayParameters& p,
                                     const std::array<f32,3>& current,const std::array<f32,3>& previous,
                                     StageProjectileSegment* out,std::string* error){
    using namespace source_intersection_detail;
    auto fail=[&](const char* why){if(error)*error=std::string("stage projectile segment: ")+why;return false;};
    if(!out||!stage_ray_parameters_valid(p)||!valid(current)||!valid(previous))return fail("declared scene bounds and endpoints required");
    for(const auto* grid:grids)if(!grid||!grid->loaded()||f32(grid->cell_size_x())!=p.cell_size||
        f32(grid->cell_size_z())!=p.cell_size)return fail("all three loaded stage grids required");
    StageProjectileSegment next;next.position=current;
    if(!stage_projectile_segment_detail::clip(current,previous,p,&next.clipped_current)||
       !stage_projectile_segment_detail::clip(previous,current,p,&next.clipped_previous))return fail("endpoint clip domain");
    const auto delta=sub(next.clipped_current,next.clipped_previous);
    if(dot(delta,delta)>bits_to_f32(0x358637bdu)){
        next.queried=true;
        // 841D4 uses the previous-endpoint scratch (r31), even though
        // the current endpoint was clipped first.
        if(!stage_all_segment_query(grids,p,next.clipped_previous,next.clipped_current,&next.surface,error))return false;
        if(next.surface.grid_code)next.position=next.surface.nearest.point;
    }
    *out=next;if(error)error->clear();return true;
}

struct StageProjectileBounds {
    StageRayParameters horizontal;
    f32 top34=0;
    bool top_declared=false;
};
// 83244 point test, after a clear segment result. The X/Z margins and
// asymmetric vertical limits are not the earlier segment clip rectangle.
inline bool stage_projectile_outside(const StageProjectileBounds& bounds,const std::array<f32,3>& position,bool* out){
    using namespace source_intersection_detail;
    const auto& p=bounds.horizontal;
    if(!out||!bounds.top_declared||!stage_ray_parameters_valid(p)||!valid(position)||
       !std::isfinite(bounds.top34)||std::abs(bounds.top34)>1e7f)return false;
    *out=position[0]<f32(p.min_x-750.f)||position[0]>f32(p.max_x+750.f)||
        position[2]<f32(p.min_z-750.f)||position[2]>f32(p.max_z+750.f)||
        position[1]>f32(bounds.top34+750.f)||position[1]< -1000.f;
    return true;
}
}
