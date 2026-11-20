#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_flight.hpp"

namespace gf {
struct GRedBeamScalePlan {
    f32 reference188=0,physical_c0=0,relative_c4=0,length184=0;
    std::array<f32,3> scale58{};
};
namespace gred_beam_transform_detail {
inline bool scale_fields(f32 reference,f32 current,f32 distance,GRedBeamScalePlan* out){
    using source_basis_detail::number;
    if(!out||!number(reference)||reference<=0||!number(current)||current<=0||!number(distance)||distance<0)return false;
    GRedBeamScalePlan next;
    next.reference188=reference;next.physical_c0=current;next.relative_c4=f32(current/reference);
    next.length184=std::min(distance,1000.f);
    next.scale58={current,current,f32(next.length184/22.f)};
    if(!number(next.relative_c4))return false;
    *out=next;return true;
}
inline bool matrix(const Mat4& m){
    for(f32 x:m.m)if(!source_basis_detail::number(x))return false;
    return m.at(3,0)==0&&m.at(3,1)==0&&m.at(3,2)==0&&m.at(3,3)==1;
}
}
// Exact arg-43 scale operands at C3F4C..C3FBC. The parent scalar is
// separate from the key-7 affine matrix and from the beam's length.
inline bool normal_gred_beam_scale_seed(NormalGRedNeutralBSource source,f32 parent_b4,f32 distance180,
                                        GRedBeamScalePlan* out){
    const f32 reference=f32(f32(40.f/bits_to_f32(0x4213d70au))*parent_b4);
    return source.reviewed()&&gred_beam_transform_detail::scale_fields(reference,reference,distance180,out);
}
// C4208..4254, only when the caller declares the unmodified B1 path.
// No modifier callback or live shape scale is advanced by this helper.
inline bool normal_gred_beam_scale_step(NormalGRedNeutralBSource source,u8 modifiers_b1,f32 reference188,
                                        f32 current_b4,f32 distance180,GRedBeamScalePlan* out){
    return source.reviewed()&&!modifiers_b1&&
        gred_beam_transform_detail::scale_fields(reference188,current_b4,distance180,out);
}
enum class GRedBeamBasisRoute:u8 {UpReference,SideReference,Retained};
struct GRedBeamTransformPlan {
    Mat4 frame114;
    GRedBeamBasisRoute basis=GRedBeamBasisRoute::UpReference;
};
// C44A8 uses current20-origin174, not motion38. It tries the shared
// 45EF4 mode 5/+Y recipe, then mode 4/+X. If neither writes columns,
// the source still applies its explicit scales to the retained basis.
// This is a numeric candidate, not a current scene or collider transform.
inline bool normal_gred_beam_transform(NormalGRedNeutralBSource source,const GRedBeamFlightSample& flight,
                                        const GRedBeamScalePlan& scale,const Mat4& prior114,GRedBeamTransformPlan* out){
    using namespace source_intersection_detail;
    using gred_beam_transform_detail::matrix;
    if(!out||!source.reviewed()||!gred_beam_flight_detail::valid(flight)||!matrix(prior114)||
       !valid(scale.scale58)||scale.scale58[0]<=0||scale.scale58[1]!=scale.scale58[0]||scale.scale58[2]<0)return false;
    GRedBeamTransformPlan next;
    auto result=prior114;const auto direction=sub(flight.position20,flight.origin174);
    constexpr f32 threshold=bits_to_f32(0x2edbe6feu);
    bool built=false;
    if(!source_basis_detail::basis(5,direction,{0,1,0},threshold,&result,&built))return false;
    if(!built){
        if(!source_basis_detail::basis(4,direction,{1,0,0},threshold,&result,&built))return false;
        next.basis=built?GRedBeamBasisRoute::SideReference:GRedBeamBasisRoute::Retained;
    }
    next.frame114=mat4_multiply(result,mat4_scale(scale.scale58.data()));
    for(unsigned i=0;i<3;++i)next.frame114.at(i,3)=flight.position20[i];
    if(!matrix(next.frame114))return false;
    *out=next;return true;
}
}
