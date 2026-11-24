#pragma once
//80047AA4 parent-logical-bone branch ONLY. Pure numeric consumer, not child
// allocation/update scheduling, model rendering, lifetime or startup admission.
#include <array>
#include <span>
#include <string>
#include "engine/core/source_heading.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct AttachmentBoneTransformInput {
    u8 flags=0; //84: require40(copy bone)+1(local position); rotations8/4/2, scale10
    u8 bone=0; //89: logical parent slot, NOT attachment-model joint order
    std::array<f32,3> local_position{}; //64/68/6C
    std::array<s16,3> angles{}; //70/72/74, signed binary angles
    std::array<f32,3> scale{1,1,1}; //58/5C/60, consumed only when flag10 is set
    bool operator==(const AttachmentBoneTransformInput&)const=default;
};
struct AttachmentBoneTransform {
    Mat4 world; //114: copied parent basis, transformed offset, then localZ/Y/X/S
    std::array<f32,3> position{}; //20/24/28: updated BEFORE rotations/scaling
    bool valid=false;
};
// Per-slot presentation state (not a controller clock). An absent or rewound
// slot starts from neutral; revisiting the same published tick cannot turn it.
struct AttachmentMountTurn {
    std::array<s16,3> angles{};
    u32 tick=0;
    bool started=false;
    std::array<f32,3> sample(const std::array<f32,3>& target,const std::array<u16,3>& step,u32 now){
        if(started&&now<tick)*this={};
        const bool advance=!started||now!=tick;
        std::array<f32,3> result=target;
        for(unsigned k=0;k<3;++k)if(step[k]){
            // The package validates target in ±180 before this evaluator.
            if(advance){
                const auto desired=source_heading_wrap(s32(std::lround(f64(target[k])*(32768./180.))));
                const auto delta=source_heading_wrap(s32(desired)-angles[k]);
                const s32 limit=step[k];
                angles[k]=source_heading_wrap(s32(angles[k])+std::clamp<s32>(delta,-limit,limit));
            }
            result[k]=f32(angles[k])*(180.f/32768.f);
        }
        tick=now;started=true;return result;
    }
};
namespace attachment_transform_detail {
inline bool affine(const Mat4& matrix){
    for(auto v:matrix.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    return matrix.m[12]==0&&matrix.m[13]==0&&matrix.m[14]==0&&matrix.m[15]==1;
}
inline bool identity_rotation(const std::array<f32,3>& position,const std::array<s16,3>& angles,AttachmentBoneTransform* out,std::string* error){
    if(!out)return false;
    for(f32 v:position)if(!std::isfinite(v)||std::abs(v)>1e4f){if(error)*error="attachment identity transform: position domain";return false;}
    AttachmentBoneTransform next;next.position=position;next.world=mat4_translation(position.data());
    const f32 unit=bits_to_f32(0x38c90fdbu);
    next.world=mat4_multiply(next.world,mat4_rotation_z(f32(angles[2])*unit));
    next.world=mat4_multiply(next.world,mat4_rotation_y(f32(angles[1])*unit));
    next.world=mat4_multiply(next.world,mat4_rotation_x(f32(angles[0])*unit));
    if(!affine(next.world)){if(error)*error="attachment identity transform: derived matrix domain";return false;}
    next.valid=true;*out=next;if(error)error->clear();return true;
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
// Authored mount, with explicit local X/Y/Z concatenation. Do not change the
// older source-qualified 47AA4 Z/Y/X entry or treat a mount as a logical bone.
inline bool attachment_mount_transform(std::span<const Mat4> parent_bones,u8 bone,
                                        const std::array<f32,3>& local,const std::array<f32,3>& degrees,
                                        AttachmentBoneTransform* out,std::string* error,bool zyx=false){
    if(out)*out={};
    if(!out)return false;
    AttachmentBoneTransformInput input;input.flags=0x41;input.bone=bone;input.local_position=local;
    AttachmentBoneTransform next;
    if(!attachment_bone_transform(parent_bones,input,&next,error))return false;
    for(auto angle:degrees)if(!std::isfinite(angle)||std::abs(angle)>180){
        if(error)*error="attachment mount: local angle domain";
        return false;
    }
    constexpr f32 radians=3.14159265358979323846f/180.f;
    const auto x=[&](){if(degrees[0])next.world=mat4_multiply(next.world,mat4_rotation_x(degrees[0]*radians));};
    const auto z=[&](){if(degrees[2])next.world=mat4_multiply(next.world,mat4_rotation_z(degrees[2]*radians));};
    if(zyx)z();else x();
    if(degrees[1])next.world=mat4_multiply(next.world,mat4_rotation_y(degrees[1]*radians));
    if(zyx)x();else z();
    if(!attachment_transform_detail::affine(next.world)){
        if(error)*error="attachment mount: derived affine matrix domain";
        return false;
    }
    *out=next;return true;
}
// Separate cosmetic mount: the bone supplies a point, never the axes or scale.
inline bool attachment_world_axes_mount_transform(std::span<const Mat4> bones,u8 bone,
                                                   const std::array<f32,3>& local,const std::array<f32,3>& degrees,
                                                   AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    if(!out)return false;
    auto fail=[&](){if(error)*error="attachment mount: world-axes placement domain";return false;};
    for(auto angle:degrees)if(!std::isfinite(angle)||std::abs(angle)>180)return fail();
    AttachmentBoneTransformInput input;input.flags=0x41;input.bone=bone;input.local_position=local;
    AttachmentBoneTransform next;
    if(!attachment_bone_transform(bones,input,&next,error))return false;
    constexpr f32 radians=3.14159265358979323846f/180.f;
    const f32 angles[]{degrees[0]*radians,degrees[1]*radians,degrees[2]*radians};
    const f32 scale[]{1,1,1};
    next.world=mat4_from_srt(angles,scale,next.position.data());
    if(!attachment_transform_detail::affine(next.world))return fail();
    *out=next;return true;
}
// Separate80047AA4 identity-base branch used by source84==0F props. Never fake
// a parent bone to enter the older copied-bone API; its refusals stay unchanged.
inline bool attachment_identity_transform(const AttachmentBoneTransformInput& input,
                                          AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("attachment identity transform: ")+why;return false;};
    if(!out||input.flags!=0x0f||input.bone)return fail("explicit identity/local-position/all-rotation branch only");
    return attachment_transform_detail::identity_rotation(input.local_position,input.angles,out,error);
}
//47AA4 flags0E: translation is read directly from20/24/28, not local64/68/6C.
struct AttachmentWorldPositionInput {u8 flags=0x0e;std::array<f32,3> position20{};std::array<s16,3> angles{};};
inline bool attachment_world_position_transform(const AttachmentWorldPositionInput& input,AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    if(!out||input.flags!=0x0e){if(error)*error="attachment world-position transform: explicit0E branch required";return false;}
    return attachment_transform_detail::identity_rotation(input.position20,input.angles,out,error);
}
//Separate47AA4 flags1E entry: direct20 position, Z/Y/X rotations, then58 scale.
//Keep the old0E and copied-bone entry refusals; no fake parent bone is supplied.
struct AttachmentScaledWorldPositionInput {u8 flags=0x1e;std::array<f32,3> position20{},scale58{1,1,1};std::array<s16,3> angles{};};
inline bool attachment_scaled_world_position_transform(const AttachmentScaledWorldPositionInput& input,AttachmentBoneTransform* out,std::string* error){
    if(out)*out={};
    if(!out||input.flags!=0x1e){if(error)*error="attachment scaled world-position: explicit1E branch required";return false;}
    for(f32 value:input.scale58)if(!std::isfinite(value)||std::abs(value)>1e4f){if(error)*error="attachment scaled world-position: scale domain";return false;}
    AttachmentBoneTransform next;if(!attachment_transform_detail::identity_rotation(input.position20,input.angles,&next,error))return false;
    next.world=mat4_multiply(next.world,mat4_scale(input.scale58.data()));
    if(!attachment_transform_detail::affine(next.world)){if(error)*error="attachment scaled world-position: derived affine domain";return false;}
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
