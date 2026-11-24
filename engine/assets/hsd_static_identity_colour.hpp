#pragma once
#include "engine/assets/hsd.hpp"
namespace gf {
enum class HsdPublishedMaterialPolicy {Strict,StaticIdentityColourSubtract};
//8023F280: D-white - mix(A-one,B-black,C-texture), clamped/unity RGB;
//alpha activation is absent. This reviewed immutable expression reduces to
//the original TEXC, not a general custom/global TEV evaluator.
inline bool hsd_static_identity_colour(const hsd_detail::Reader& r,const HsdMeshProperties& original,HsdMeshProperties* out,std::string* error){
    auto fail=[&](){if(error)*error="static identity colour: exact immutable single-stage source expression required";return false;};
    using hsd_detail::be32;
    const u32 m=original.material_offset;
    const bool modes=(original.texture_colour_mode==4&&original.texture_alpha_mode==3)||(original.texture_colour_mode==3&&original.texture_alpha_mode==2);
    if(!out||original.material_supported||original.static_diffuse_overwrite||original.texture==kHsdNoTexture||original.texture_uv1||original.texture_colour_lerp||original.texture_stage_inactive||original.texture_matrix_mask||original.texture_matrix_required||original.generated_uv_required||original.texture_after_lighting||!modes||f32_to_bits(original.texture_weight)!=0x3f800000||!r.has(m,24))return fail();
    auto pointer=[&](u32 at,u32* value){if(!r.has(at,4))return false;const auto raw=be32(r.block,at);if(!raw||raw==0xffffffffu){*value=0;return true;}return r.ptr(at,value);};
    u32 klass=0,t=0,render=0;
    if(!pointer(m,&klass)||klass||!pointer(m+8,&t)||!t||!pointer(m+16,&render)||render||(be32(r.block,m+4)&0x04001000u)||!r.has(t,92))return fail();
    u32 next=0,tev=0;
    const u32 expected_flags=original.texture_colour_mode==4?0x00340010u:0x00230010u;
    if(!pointer(t,&klass)||klass||!pointer(t+4,&next)||next||be32(r.block,t+12)!=4||be32(r.block,t+64)!=expected_flags||be32(r.block,t+68)!=0x3f800000||!pointer(t+88,&tev)||!tev||!r.has(tev,32))return fail();
    for(unsigned k=0;k<3;++k)if(r.block.bef32(t+16+4*k)!=0||r.block.bef32(t+28+4*k)!=1||r.block.bef32(t+40+4*k)!=0)return fail();
    const std::array<u8,16> expression{1,0,0,0,0,0,1,1,12,0x87,8,0x85,7,7,7,7};
    if(!std::equal(expression.begin(),expression.end(),r.block.data+tev)||be32(r.block,tev+16)||be32(r.block,tev+20)!=0xffffff00||be32(r.block,tev+24)||be32(r.block,tev+28)!=0x40000770)return fail();
    *out=original;out->material_supported=true;if(error)error->clear();return true;
}
}
