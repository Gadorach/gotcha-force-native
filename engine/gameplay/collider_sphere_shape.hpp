#pragma once
#include <optional>
#include "engine/gameplay/collider_plane_shape.hpp"

namespace gf {
struct ColliderSphereSweepSeed {
    std::array<f32,3> start8c{},end98{},previous_start_d4{},previous_end_e0{};
    bool operator==(const ColliderSphereSweepSeed&)const=default;
};
struct ColliderSphereSeed {
    std::array<f32,3> center64{},previous_center70{};
    f32 radius88=0,broad_radius7c=0;
    // These are two exclusive interpretations of the source fields at 8C.
    std::optional<f32> previous_radius8c;
    std::optional<ColliderSphereSweepSeed> sweep;
    bool operator==(const ColliderSphereSeed&)const=default;
};

// Shape-0 numeric writes in 3A4B8. The shape still needs its enclosing
// 8A65C lifecycle, parameter effects and registry visit.
inline bool collider_sphere_seed(const CharacterCollisionDescriptor& descriptor,const Mat4& frame,f32 carrier_c0,
                                 ColliderSphereSeed* out){
    using namespace collider_plane_detail;
    if(!out||!descriptor.declared||descriptor.shape||!matrix(frame)||!source_intersection_detail::valid(descriptor.local_point)||
       !number(carrier_c0)||carrier_c0<0||carrier_c0>1e4f)return false;
    const f32 local_radius=descriptor.scalar(0x18);
    if(!number(local_radius)||local_radius<0)return false;
    ColliderSphereSeed next;
    next.previous_center70=next.center64=point(frame,descriptor.local_point);
    next.radius88=f32(local_radius*carrier_c0);
    next.broad_radius7c=f32((descriptor.flags2&0x10u?50.f:local_radius)*carrier_c0);
    if(!source_intersection_detail::valid(next.center64)||!number(next.radius88)||!number(next.broad_radius7c))return false;
    if(descriptor.flags2&0x81u){
        const auto& center=next.center64;
        next.sweep=ColliderSphereSweepSeed{center,center,center,center};
    }else next.previous_radius8c=next.radius88;
    *out=next;return true;
}
}
