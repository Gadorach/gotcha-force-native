#pragma once
// Shared field updates, not source cache allocation/TEV callback or GX hardware parity.
#include "engine/assets/attachment_alternate.hpp"
#include "engine/assets/hsd.hpp"
namespace gf {
struct AttachmentAlternateState {
    u32 joint_flags=0,dobj_flags=0,render_mode=0;bool has_pe=false;
    std::array<u8,12> pe{};f32 alpha=1;
};
inline bool attachment_alternate_apply(const AttachmentAlternateProfile& p,const AttachmentAlternateState& input,
                                       AttachmentAlternateState* out,std::string* error){
    if(out)*out={};
    if(!out||!attachment_alternate_profile_valid(p)||(input.joint_flags&0x1000u)){
        if(error)*error="attachment alternate: profile/destination/instance branch";
        return false;
    }
    auto next=input;next.joint_flags=(input.joint_flags|p.words[0])&~p.words[1];next.dobj_flags=(input.dobj_flags|p.words[2])&~p.words[3];next.render_mode=(input.render_mode|p.words[4])&~p.words[5];
    if(next.has_pe){next.pe[0]=u8(next.pe[0]&~p.words[6]);next.pe[9]=u8(p.words[7]);}
    next.alpha=p.alpha;*out=next;if(error)error->clear();return true;
}
inline bool attachment_alternate_properties(const HsdMeshProperties& base,const AttachmentAlternateState& state,HsdMeshProperties* out,std::string* error){
    if(!out){if(error)*error="attachment alternate: mesh destination";return false;}
    HsdMeshProperties mesh=base;mesh.render_mode=state.render_mode;mesh.material_alpha=state.alpha;
    //802411CC: explicit PE wins over render-mode defaults, including blending.
    if(state.has_pe){const auto& s=state.pe;
        if((s[0]&0x44u)||s[4]>1||s[5]>7||s[6]>7||s[8]>7||s[9]>7||s[10]>3||s[11]>7){if(error)*error="attachment alternate: unsupported destination alpha/dither/PE state";return false;}
        mesh.blend=s[4]==1;mesh.blend_src=u8(s[5]+1);mesh.blend_dst=u8(s[6]+1);mesh.depth_test=(s[0]&16)!=0;mesh.depth_write=(s[0]&32)!=0;mesh.colour_write=(s[0]&1)!=0;mesh.alpha_write=(s[0]&2)!=0;
        static constexpr u8 reverse_compare[]={0,4,2,6,1,5,3,7};mesh.depth_compare=reverse_compare[s[8]];mesh.alpha_compare0=s[9];mesh.alpha_compare1=s[11];mesh.alpha_op=s[10];mesh.alpha_ref0=s[1];mesh.alpha_ref1=s[2];
    }else{
        mesh.blend=(state.render_mode&0x40000000u)!=0;mesh.blend_src=5;mesh.blend_dst=6;mesh.depth_test=true;mesh.depth_write=(state.render_mode&0x20000000u)==0;mesh.depth_compare=(state.render_mode&0x08000000u)?7:6;mesh.colour_write=true;mesh.alpha_write=false;
        mesh.alpha_compare0=mesh.alpha_compare1=(mesh.blend&&mesh.depth_write)?4:7;mesh.alpha_op=0;mesh.alpha_ref0=mesh.alpha_ref1=0;
    }
    *out=std::move(mesh);if(error)error->clear();return true;
}
inline bool attachment_alternate_mesh(const HsdMesh& base,const AttachmentAlternateState& state,HsdMesh* out,std::string* error){
    if(!out){if(error)*error="attachment alternate: mesh destination";return false;}
    HsdMesh mesh=base;
    if(!attachment_alternate_properties(base,state,&mesh,error))return false;
    *out=std::move(mesh);return true;
}
} // namespace gf
