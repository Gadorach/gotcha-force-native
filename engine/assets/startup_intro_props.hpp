#pragma once
// Offline-bound stff kind0 PATH and kind2 alpha-replacement operands.
#include "engine/assets/hsd_record_animation.hpp"
namespace gf {
struct StartupIntroPropProfile {
    u32 bank_hash=0;u16 bank_records=0,path_record=0,fade_record=0;u32 path_root=0,fade_root=0;
    f32 first_frame=0,rate=0,alpha_start=0,alpha_step=0,zero=0;bool loaded=false;
};
inline bool startup_intro_prop_profile_valid(const StartupIntroPropProfile& p){
    return p.loaded&&p.bank_hash&&p.bank_records>=2&&p.bank_records<=4096&&p.path_record<p.bank_records&&p.fade_record<p.bank_records&&p.path_record!=p.fade_record&&p.path_root&&p.fade_root&&p.path_root!=p.fade_root&&p.path_root<(64u<<20)&&p.fade_root<(64u<<20)&&f32_to_bits(p.first_frame)==0&&f32_to_bits(p.rate)==0x3f800000u&&f32_to_bits(p.alpha_start)==0x3f800000u&&f32_to_bits(p.alpha_step)==0x3dcccccdu&&f32_to_bits(p.zero)==0;
}
inline bool startup_intro_prop_load(ByteView file,StartupIntroPropProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup intro prop profile: ")+why;return false;};
    if(!out||!file.data||file.size!=64||file.be32(0)!=0x47464954u||file.be16(4)!=1||file.be16(6)!=2||file.be32(8)!=64||file.be16(22)||file.data[52]!=2||file.data[53]!=1||file.data[54]!=0x28||file.data[55]!=0x47||file.data[56]!=0x44||file.data[57]||file.be16(58)||file.be32(60))return fail("header/extent/policy");
    StartupIntroPropProfile p;p.bank_hash=file.be32(12);p.bank_records=file.be16(16);p.path_record=file.be16(18);p.fade_record=file.be16(20);p.path_root=file.be32(24);p.fade_root=file.be32(28);p.first_frame=file.bef32(32);p.rate=file.bef32(36);p.alpha_start=file.bef32(40);p.alpha_step=file.bef32(44);p.zero=file.bef32(48);p.loaded=true;
    if(!startup_intro_prop_profile_valid(p))return fail("source operand/resource domain");
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
