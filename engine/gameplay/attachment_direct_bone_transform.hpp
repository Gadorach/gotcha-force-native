#pragma once
#include "engine/gameplay/attachment_transform.hpp"
namespace gf {
//47AA4's copied-bone BASIS + direct20 translation path. Do not fake a local64
//offset or broaden the older41/0F/1E entry points to accept these flags.
struct AttachmentDirectBoneTransformInput {
    u8 flags=0x54,bone=0;std::array<f32,3> position20{},scale58{1,1,1};std::array<s16,3> angles{};
};
inline bool attachment_direct_bone_transform(std::span<const Mat4> parent_bones,const AttachmentDirectBoneTransformInput& input,AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="direct bone transform: exact54/current bounded bone/position required";return false;};
    if(!out||input.flags!=0x54||parent_bones.empty()||parent_bones.size()>32||input.bone>=parent_bones.size()||!attachment_transform_detail::affine(parent_bones[input.bone]))return fail();
    for(f32 value:input.position20)if(!std::isfinite(value)||std::abs(value)>1e4f)return fail();
    for(f32 value:input.scale58)if(!std::isfinite(value)||std::abs(value)>1e4f)return fail();
    AttachmentBoneTransform next;next.world=parent_bones[input.bone];next.position=input.position20;
    for(unsigned k=0;k<3;++k)next.world.at(k,3)=input.position20[k];
    const f32 unit=bits_to_f32(0x38c90fdb);
    next.world=mat4_multiply(next.world,mat4_rotation_y(f32(input.angles[1])*unit));
    next.world=mat4_multiply(next.world,mat4_scale(input.scale58.data()));
    if(!attachment_transform_detail::affine(next.world))return fail();
    next.valid=true;*out=next;if(error){error->clear();}return true;
}
}
