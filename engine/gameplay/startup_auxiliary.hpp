#pragma once
// C74EC construction writes + C76A0/C77EC/C78F8/C790C and NON-TURN C79F8.
// Parent observations explicit;40 active direction/history alignment REFUSES.
#include <span>
#include "engine/assets/startup_auxiliary.hpp"
#include "engine/gameplay/attachment_transform.hpp"
namespace gf {
struct StartupAuxiliaryParent {u16 model94=0;u8 phase18=1,kind83=0,active_count=0,stored_index=0;Mat4 object_world;std::span<const Mat4> bones;};
struct StartupAuxiliaryState {
    bool allocated=false,matrices_valid=false;u16 captured94=0;s16 counter=0;u8 phase=0,render_group=0,cached_a8=0,previous_a9=0;
    std::array<Mat4,2> matrices{};std::array<std::array<f32,3>,2> history{};
};
struct StartupAuxiliaryConstruction {StartupAuxiliaryState state;u8 parent708=0;bool created=false,parent_write=false;};
inline bool startup_auxiliary_construct(const StartupAuxiliaryProfile& p,unsigned entry,const StartupAuxiliaryParent& parent,
                                        u8 scene_mode,bool allocation_available,u8 parent708,StartupAuxiliaryConstruction* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup auxiliary construct: ")+why;return false;};
    if(!out||!startup_auxiliary_profile_valid(p)||entry>1||parent.model94!=p.parent_model||parent.stored_index>1)return fail("profile/parent/entry");
    StartupAuxiliaryConstruction r;r.parent708=parent708;
    if(scene_mode!=5&&allocation_available){r.created=true;r.state.allocated=true;r.state.captured94=parent.model94;
        if(parent.kind83==0||parent.kind83==0x18){r.parent708=u8(parent708|(1u<<p.entries[entry].channel));r.parent_write=true;}}
    *out=r;if(error)error->clear();return true;
}
inline bool startup_auxiliary_step(const StartupAuxiliaryProfile& p,unsigned entry,const StartupAuxiliaryParent& parent,
                                   const StartupAuxiliaryState& input,StartupAuxiliaryState* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup auxiliary step: ")+why;return false;};
    if(!out||!startup_auxiliary_profile_valid(p)||entry>1||parent.stored_index>1||input.phase>3||(!input.allocated&&input.phase!=0&&input.phase!=3))return fail("state/profile/parent");
    auto s=input;if(!s.allocated){*out=s;if(error)error->clear();return true;}
    if(s.phase==0){s.phase=1;s.counter=0;s.cached_a8=s.previous_a9=0;s.render_group=1;} //inline2 matrices; no fake immediate77EC call
    else if(s.phase==1){s.counter=std::bit_cast<s16>(u16(u16(s.counter)+1u));
        if(parent.active_count){s.previous_a9=s.cached_a8;if(s.cached_a8){if(!s.matrices_valid)return fail("previous matrices required by source history copy");
            for(unsigned n=0;n<2;++n){f64 length=0;for(unsigned k=0;k<3;++k){const f32 v=s.matrices[n].at(k,1);if(!std::isfinite(v))return fail("history matrix finite");length+=f64(v)*v;}length=std::sqrt(length);if(!std::isfinite(length)||length<1e-12)return fail("source history normalization domain");for(unsigned k=0;k<3;++k)s.history[n][k]=f32(s.matrices[n].at(k,1)/length);}}}
        s.cached_a8=parent.active_count;
        if(std::bit_cast<s8>(parent.phase18)>1||parent.model94!=s.captured94){s.phase=2;s.render_group=0;}
    }else if(s.phase==2){s.phase=3;s.render_group=0;}
    else{s.allocated=false;}
    // C77EC/C78F8 hide82 but do not unregister100.89554 still runs C79F8
    // in hidden phases2/3; C790C release unregisters before this stage.
    if(s.allocated&&parent.active_count){const auto& e=p.entries[entry];
        for(unsigned n=0;n<2;++n){const auto& d=e.parts[n];if(d.turn)return fail("80045460/80045BC8 direction/history alignment required");Mat4 m;
            if(parent.kind83==0&&d.bone>=0){if(parent.bones.empty()||parent.bones.size()>32||unsigned(d.bone)>=parent.bones.size())return fail("logical parent bone");m=parent.bones[unsigned(d.bone)];}else m=parent.object_world;
            if(!attachment_transform_detail::affine(m))return fail("parent affine matrix domain");
            m=mat4_multiply(m,mat4_translation(d.position.data()));m=mat4_multiply(m,mat4_rotation_x(f32(d.angle_x)*p.angle_unit));m=mat4_multiply(m,mat4_rotation_z(f32(d.angle_z)*p.angle_unit));
            const unsigned pulse=((u32(s32(s.counter))+n)>>1)&1u;std::array<f32,3> scale;for(unsigned k=0;k<3;++k)scale[k]=f32(p.pulse[pulse][k]*d.size);m=mat4_multiply(m,mat4_scale(scale.data()));
            if(!attachment_transform_detail::affine(m))return fail("derived auxiliary world domain");
            s.matrices[n]=m;
        }
        s.matrices_valid=true;
    }
    *out=s;if(error)error->clear();return true;
}
} // namespace gf
