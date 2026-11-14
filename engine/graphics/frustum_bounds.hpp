#pragma once
// Conservative bounds rejection before vertex shading. Interval operations
// round outwards at every float operation, including camera subtraction and
// plane products. Borderline/nonfinite bounds remain visible for the existing
// triangle clipper; this never substitutes an approximate clipped polygon.
#include <algorithm>
#include <cmath>
#include <limits>
#include "engine/graphics/camera.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct Bounds3 {
    f32 low[3]={INFINITY,INFINITY,INFINITY};
    f32 high[3]={-INFINITY,-INFINITY,-INFINITY};
    bool invalid=false;
    void include(const f32 p[3]){
        for(unsigned k=0;k<3;++k){invalid|=!std::isfinite(p[k]);low[k]=std::min(low[k],p[k]);high[k]=std::max(high[k],p[k]);}
    }
};
namespace frustum_bounds_detail {
struct Range {f32 low,high;};
inline Range outward(f64 low,f64 high){
    return {std::nextafter(f32(low),-std::numeric_limits<f32>::infinity()),
            std::nextafter(f32(high),std::numeric_limits<f32>::infinity())};
}
inline Range add(Range a,Range b){return outward(f64(a.low)+b.low,f64(a.high)+b.high);}
inline Range multiply(Range a,f32 b){
    const f64 x=f64(a.low)*b,y=f64(a.high)*b;
    return outward(std::min(x,y),std::max(x,y));
}
inline Range dot(const Range a[3],const f32 b[3]){
    return add(add(multiply(a[0],b[0]),multiply(a[1],b[1])),multiply(a[2],b[2]));
}
}
inline bool frustum_bounds_visible(const Bounds3& bounds,const Mat4& matrix,
                                   const Camera& camera,const Camera::Basis& basis){
    using namespace frustum_bounds_detail;
    if(bounds.invalid)return true;
    Range local[3],relative[3];
    for(unsigned k=0;k<3;++k){
        if(!std::isfinite(bounds.low[k])||!std::isfinite(bounds.high[k])||bounds.low[k]>bounds.high[k])return true;
        local[k]={bounds.low[k],bounds.high[k]};
    }
    for(unsigned row=0;row<3;++row){
        const f32 coefficients[3]={matrix.at(row,0),matrix.at(row,1),matrix.at(row,2)};
        auto world=add(dot(local,coefficients),{matrix.at(row,3),matrix.at(row,3)});
        relative[row]=add(world,{-basis.eye[row],-basis.eye[row]});
        if(!std::isfinite(relative[row].low)||!std::isfinite(relative[row].high))return true;
    }
    const Range x=dot(relative,basis.right),y=dot(relative,basis.up),z=dot(relative,basis.forward);
    const auto horizontal=multiply(z,basis.tan_half_fov*basis.aspect),vertical=multiply(z,basis.tan_half_fov);
    const Range planes[]={add(z,{-camera.near_plane,-camera.near_plane}),
        add(multiply(z,-1),{camera.far_plane,camera.far_plane}),
        add(x,horizontal),add(horizontal,multiply(x,-1)),add(y,vertical),add(vertical,multiply(y,-1))};
    for(const auto plane:planes){if(!std::isfinite(plane.low)||!std::isfinite(plane.high))return true;}
    for(const auto plane:planes)if(plane.high<0)return false;
    return true;
}
} // namespace gf
