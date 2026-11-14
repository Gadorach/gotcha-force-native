#pragma once
#include <array>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
namespace source_intersection_detail {
using Vec=std::array<f32,3>;
inline bool valid(const Vec& v){for(f32 x:v)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;return true;}
inline Vec sub(const Vec& a,const Vec& b){return {f32(a[0]-b[0]),f32(a[1]-b[1]),f32(a[2]-b[2])};}
inline Vec scale(const Vec& a,f32 b){return {f32(a[0]*b),f32(a[1]*b),f32(a[2]*b)};}
inline f32 dot(const Vec& a,const Vec& b){return f32(f32(f32(a[0]*b[0])+f32(a[1]*b[1]))+f32(a[2]*b[2]));}
inline Vec cross(const Vec& a,const Vec& b){return {f32(f32(a[1]*b[2])-f32(a[2]*b[1])),f32(f32(a[2]*b[0])-f32(a[0]*b[2])),f32(f32(a[0]*b[1])-f32(a[1]*b[0]))};}
}
struct SourceSegmentPlaneHit {std::array<f32,3> point{};u8 side=0;bool operator==(const SourceSegmentPlaneHit&)const=default;};
//39DA4: side masks1/2 cross;0/3 do NOT. Zero-distance endpoint signs are
//deliberate. Preserve the zero-denominator normal-scaled fallback, not a new
//epsilon/one-sided/Moller-Trumbore policy. Bounded float32 model, not PS parity.
inline bool source_segment_plane(const std::array<f32,3>& start,const std::array<f32,3>& end,const std::array<f32,3>& normal,const std::array<f32,3>& anchor,SourceSegmentPlaneHit* out){
    if(out)*out={};
    using namespace source_intersection_detail;if(!out||!valid(start)||!valid(end)||!valid(normal)||!valid(anchor))return false;
    const f32 a=dot(sub(start,anchor),normal),b=dot(sub(end,anchor),normal);if(!std::isfinite(a)||!std::isfinite(b))return false;
    const u8 side=u8((a>=0?1:0)|(b>=0?2:0));if(side==0||side==3)return true;
    const auto& origin=side==1?start:end;const auto delta=side==1?sub(start,end):sub(end,start);const f32 distance=side==1?a:b,denominator=dot(delta,normal);
    if(!std::isfinite(denominator))return false;
    const auto offset=denominator==0?scale(normal,distance):scale(delta,f32(distance/denominator));const auto point=sub(origin,offset);if(!valid(point))return false;*out={point,side};return true;
}
//39AEC: cross(vertex_i-point,vertex_next-point) dot normal >=0 for all3.
inline bool source_triangle_contains(const std::array<f32,3>& point,const std::array<std::array<f32,3>,3>& vertices,const std::array<f32,3>& normal,bool* out){
    if(out)*out=false;
    using namespace source_intersection_detail;if(!out||!valid(point)||!valid(normal))return false;for(const auto& v:vertices)if(!valid(v))return false;
    for(unsigned i=0;i<3;++i){const f32 value=dot(cross(sub(vertices[i],point),sub(vertices[(i+1)%3],point)),normal);if(!std::isfinite(value))return false;if(value<0)return true;}
    *out=true;return true;
}
} // namespace gf
