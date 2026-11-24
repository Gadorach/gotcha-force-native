#pragma once
#include "engine/gameplay/attachment_transform.hpp"
namespace gf {
//47AA4's identity-base/local64 branches used by initial briefing callbacks.
//Numbers only: no widget/parent ownership, callback traversal or model-world
//publication. Existing0F/1E/copied-bone entries keep their own refusals.
struct AttachmentUiLocalTransformInput {
    u8 flags84=1;std::array<f32,3> local64{},scale58{1,1,1};s16 angle74=0;
};
inline bool attachment_ui_local_transform(const AttachmentUiLocalTransformInput& input,AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="UI local transform: only identity/local64 flags01/11/19 and bounded consumed operands supported";return false;};
    if(!out||(input.flags84!=0x01&&input.flags84!=0x11&&input.flags84!=0x19))return fail();
    for(f32 v:input.local64)if(!std::isfinite(v)||std::abs(v)>1e4f)return fail();
    if(input.flags84&0x10)for(f32 v:input.scale58)if(!std::isfinite(v)||std::abs(v)>1e4f)return fail();
    AttachmentBoneTransform next;
    mat4_transform_point(next.world,input.local64.data(),next.position.data());
    for(unsigned k=0;k<3;++k)next.world.at(k,3)=next.position[k];
    if(input.flags84&8)next.world=mat4_multiply(next.world,mat4_rotation_z(f32(input.angle74)*bits_to_f32(0x38c90fdbu)));
    if(input.flags84&0x10)next.world=mat4_multiply(next.world,mat4_scale(input.scale58.data()));
    if(!attachment_transform_detail::affine(next.world))return fail();
    next.valid=true;*out=next;if(error){error->clear();}return true;
}
}
