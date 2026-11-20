#pragma once
#include "engine/gameplay/borgs/normal_gred_neutral_b.hpp"
#include "engine/graphics/source_basis.hpp"

namespace gf {
struct GRedBeamNoTargetBasis {
    u8 parent_slot=255,credit_slot=255;
    Mat4 parent_key7,credit_key0;
};
struct GRedBeamMuzzlePlan {
    std::array<f32,3> origin174{},unit_bearing{};
};

// Detached numeric subset of the arg-43 initializer, with C8/CC absent
// and direct normal G-Red parent/credit. The caller declares these
// matrices; the slot check does not establish their origin or freshness.
// In the original, registration succeeds before this initializer reaches
// the matrix reads. This prepares neither that receipt nor a live object.
inline bool normal_gred_beam_no_target_muzzle(NormalGRedNeutralBSource source,const GRedNeutralBReleasePlan& paid,
                                             bool no_copied_target,const GRedBeamNoTargetBasis& basis,GRedBeamMuzzlePlan* out){
    if(!out||!source.reviewed()||!paid.request_beam||paid.owner_slot!=source.slot||paid.object_type!=2||paid.argument!=43||
       !no_copied_target||basis.parent_slot!=source.slot||basis.credit_slot!=source.slot)return false;
    for(const auto* m:{&basis.parent_key7,&basis.credit_key0}){
        for(f32 x:m->m)if(!source_basis_detail::number(x))return false;
        if(m->at(3,0)!=0||m->at(3,1)!=0||m->at(3,2)!=0||m->at(3,3)!=1)return false;
    }
    GRedBeamMuzzlePlan next;
    // 80303CA4 + 4, in the same units as the parent key-7 matrix.
    constexpr f32 offset[]{bits_to_f32(0xc2d5999a),bits_to_f32(0xc0666666),30.f};
    mat4_transform_point(basis.parent_key7,offset,next.origin174.data());
    // 83E64 null-target fallback reads the credited key-0 Z column, not
    // the parent key-7 forward column or the actor's last aim point.
    next.unit_bearing=source_basis_detail::column(basis.credit_key0,2);
    if(!source_intersection_detail::valid(next.origin174)||!source_basis_detail::normalize(&next.unit_bearing))return false;
    *out=next;return true;
}
}
