#pragma once
//80047AA4 parent-logical-bone branch ONLY. Pure numeric consumer, not child
// allocation/update scheduling, model rendering, lifetime or startup admission.
#include <array>
#include <span>
#include <string>
#include "engine/graphics/transform.hpp"
namespace gf {
struct AttachmentBoneTransformInput {
    u8 flags=0; //84: require40(copy bone)+1(local position); rotations8/4/2, scale10
    u8 bone=0; //89: logical parent slot, NOT attachment-model joint order
    std::array<f32,3> local_position{}; //64/68/6C
    std::array<s16,3> angles{}; //70/72/74, signed binary angles
    std::array<f32,3> scale{1,1,1}; //58/5C/60, consumed only when flag10 is set
};
struct AttachmentBoneTransform {
    Mat4 world; //114: copied parent basis, transformed offset, then localZ/Y/X/S
    std::array<f32,3> position{}; //20/24/28: updated BEFORE rotations/scaling
    bool valid=false;
};
namespace attachment_transform_detail {
inline bool affine(const Mat4& matrix){
    for(auto v:matrix.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    return matrix.m[12]==0&&matrix.m[13]==0&&matrix.m[14]==0&&matrix.m[15]==1;
}
}
inline bool attachment_bone_transform(std::span<const Mat4> parent_bones,
                                      const AttachmentBoneTransformInput& input,
                                      AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("attachment bone transform: ")+why;return false;};
    if(!out)return fail("destination required");
    // Do not broaden this to identity/parent-object/world-position branches.
    if((input.flags&0xe1u)!=0x41u)return fail("only copied-bone/local-position flags supported");
    if(parent_bones.empty()||parent_bones.size()>32||input.bone>=parent_bones.size())return fail("logical parent bone/budget");
    const auto& parent=parent_bones[input.bone];
    if(!attachment_transform_detail::affine(parent))return fail("parent affine matrix domain");
    for(auto v:input.local_position)if(!std::isfinite(v)||std::abs(v)>1e4f)return fail("local position domain");
    if(input.flags&0x10u)for(auto v:input.scale)if(!std::isfinite(v)||std::abs(v)>1e4f)return fail("scale domain");
    AttachmentBoneTransform next;next.world=parent;
    mat4_transform_point(parent,input.local_position.data(),next.position.data());
    for(unsigned k=0;k<3;++k)next.world.at(k,3)=next.position[k];
    // Raw PSQ loads use W=1/GQR5 at74,72,70: Z, Y, X. Source804370E0
    // is38C90FDB. The default signed16/scale0 GQR domain is explicit here;
    // host libm/rounding is NOT a PPC hardware/FPSCR equivalence claim.
    const f32 unit=bits_to_f32(0x38c90fdbu);
    if(input.flags&8u)next.world=mat4_multiply(next.world,mat4_rotation_z(f32(input.angles[2])*unit));
    if(input.flags&4u)next.world=mat4_multiply(next.world,mat4_rotation_y(f32(input.angles[1])*unit));
    if(input.flags&2u)next.world=mat4_multiply(next.world,mat4_rotation_x(f32(input.angles[0])*unit));
    if(input.flags&0x10u)next.world=mat4_multiply(next.world,mat4_scale(input.scale.data()));
    if(!attachment_transform_detail::affine(next.world))return fail("derived affine matrix domain");
    next.valid=true;*out=next;if(error)error->clear();return true;
}
// Separate80047AA4 identity-base branch used by source84==0F props. Never fake
// a parent bone to enter the older copied-bone API; its refusals stay unchanged.
inline bool attachment_identity_transform(const AttachmentBoneTransformInput& input,
                                          AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("attachment identity transform: ")+why;return false;};
    if(!out||input.flags!=0x0f||input.bone)return fail("explicit identity/local-position/all-rotation branch only");
    for(f32 v:input.local_position)if(!std::isfinite(v)||std::abs(v)>1e4f)return fail("local position domain");
    AttachmentBoneTransform next;next.position=input.local_position;next.world=mat4_translation(next.position.data());
    const f32 unit=bits_to_f32(0x38c90fdbu);
    next.world=mat4_multiply(next.world,mat4_rotation_z(f32(input.angles[2])*unit));
    next.world=mat4_multiply(next.world,mat4_rotation_y(f32(input.angles[1])*unit));
    next.world=mat4_multiply(next.world,mat4_rotation_x(f32(input.angles[0])*unit));
    if(!attachment_transform_detail::affine(next.world))return fail("derived matrix domain");
    next.valid=true;*out=next;if(error)error->clear();return true;
}
} // namespace gf
