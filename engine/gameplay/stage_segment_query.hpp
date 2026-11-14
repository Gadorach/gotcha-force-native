#pragma once
#include "engine/assets/stage_ray.hpp"
#include "engine/assets/stih.hpp"
#include "engine/graphics/source_intersection.hpp"
namespace gf {
struct StageSegmentHit {bool hit=false,excluded_bounds=false;std::array<f32,3> point{};u32 triangle=0,cell=0,tested=0;u8 plane_side=0;bool operator==(const StageSegmentHit&)const=default;};
//3F044/3F0C0: inclusive scene X/Z bounds on BOTH endpoints, source rectangular
//cell enumeration in the secondary grid, no dedup, strictly nearest start hit.
//Success+hit=false is computed from actual input, not a missing-registry shortcut.
inline bool stage_segment_query(const StihGrid& secondary,const StageRayParameters& p,const std::array<f32,3>& start,const std::array<f32,3>& end,StageSegmentHit* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("stage segment: ")+why;return false;};using namespace source_intersection_detail;
    if(!out||!secondary.loaded()||!stage_ray_parameters_valid(p)||!valid(start)||!valid(end)||f32(secondary.cell_size_x())!=p.cell_size||f32(secondary.cell_size_z())!=p.cell_size)return fail("declared source parameters/static secondary grid/endpoints required");
    auto inside=[&](const auto& v){return p.min_x<=v[0]&&v[0]<=p.max_x&&p.min_z<=v[2]&&v[2]<=p.max_z;};if(!inside(start)||!inside(end)){out->excluded_bounds=true;if(error)error->clear();return true;}
    auto cell=[&](const auto& v,s32* x,s32* z){const f32 a=f32(f32(v[0]-f32(secondary.minimum_x()))/p.cell_size),b=f32(f32(v[2]-f32(secondary.minimum_z()))/p.cell_size);if(!std::isfinite(a)||!std::isfinite(b)||a<=-1||b<=-1||a>=f32(secondary.cells_x())||b>=f32(secondary.cells_z()))return false;*x=s32(a);*z=s32(b);return true;};
    s32 x0=0,x1=0,z0=0,z1=0;if(!cell(start,&x0,&z0)||!cell(end,&x1,&z1))return fail("bounded cell extent");if(x1<x0)std::swap(x0,x1);if(z1<z0)std::swap(z0,z1);StageSegmentHit best;f32 distance=0;
    for(s32 x=x0;x<=x1;++x)for(s32 z=z0;z<=z1;++z){const auto refs=secondary.cell(unsigned(x),unsigned(z));for(u32 index:refs){if(best.tested>=1048576)return fail("native triangle budget");++best.tested;if(index>=secondary.triangles().size())return fail("triangle index");const auto& t=secondary.triangles()[index];const Vec normal{t.plane[0],t.plane[1],t.plane[2]};SourceSegmentPlaneHit point;if(!source_segment_plane(start,end,normal,t.vertices[0],&point))return fail("segment/plane numeric domain");if(!point.side)continue;bool contained=false;if(!source_triangle_contains(point.point,t.vertices,normal,&contained))return fail("triangle numeric domain");if(!contained)continue;const auto delta=sub(start,point.point);const f32 squared=dot(delta,delta);if(!std::isfinite(squared))return fail("distance domain");if(!best.hit||squared<distance){best.hit=true;best.point=point.point;best.triangle=index;best.cell=u32(x)*secondary.cells_z()+u32(z);best.plane_side=point.side;distance=squared;}}}
    *out=best;if(error)error->clear();return true;
}
} // namespace gf
