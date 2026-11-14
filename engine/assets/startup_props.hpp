#pragma once
// Source-bound eight moving stff records; source55/path and65/fade not included.
#include <array>
#include "engine/assets/hsd_record_animation.hpp"
namespace gf {
struct StartupPropEntry {
    u16 record=0;u32 root=0;s16 delay=0,sound=-1,spin_delta=0,initial_spin=0;
    f32 height=0,velocity=0,acceleration=0,target=0;
};
struct StartupPropProfile {
    u32 bank_hash=0;u16 bank_records=0;std::array<StartupPropEntry,8> parts{};
    std::array<f32,8> settle{};f32 zero=0,radians_to_angles=0,angle_unit=0;bool loaded=false;
};
inline bool startup_prop_profile_valid(const StartupPropProfile& p){
    if(!p.loaded||!p.bank_hash||p.bank_records<8||p.bank_records>4096||f32_to_bits(p.zero)!=0||f32_to_bits(p.radians_to_angles)!=0x4622f983u||f32_to_bits(p.angle_unit)!=0x38c90fdbu)return false;
    for(unsigned i=0;i<8;++i){const auto& e=p.parts[i];if(e.record>=p.bank_records||!e.root||e.root>=(64u<<20)||e.delay<0||e.sound< -1)return false;
        for(f32 v:{e.height,e.velocity,e.acceleration,e.target})if(!std::isfinite(v)||std::abs(v)>1e4f)return false;
        if(e.height<e.target)return false;
        for(unsigned j=0;j<i;++j)if(e.record==p.parts[j].record||e.root==p.parts[j].root)return false;
        if(!std::isfinite(p.settle[i])||std::abs(p.settle[i])>1e4f)return false;
    }
    return true;
}
inline bool startup_prop_load(ByteView file,StartupPropProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup prop profile: ")+why;return false;};
    if(!out||!file.data||file.size!=352||file.be32(0)!=0x47465050u||file.be16(4)!=1||file.be16(6)!=8||file.be32(8)!=352||file.be16(18)!=32||file.be32(20)!=64||file.be32(24)!=320||file.be16(28)!=8||file.be16(30)!=30||file.be32(44)!=15||file.be32(48)!=0x20000||file.data[52]!=8||file.data[53]!=10||file.data[54]||file.data[55]!=2||file.data[56]!=0x28||file.data[57]!=1||file.be16(58)||file.be32(60))return fail("header/extent/source-policy operands");
    StartupPropProfile p;p.bank_hash=file.be32(12);p.bank_records=file.be16(16);p.zero=file.bef32(32);p.radians_to_angles=file.bef32(36);p.angle_unit=file.bef32(40);
    for(unsigned i=0;i<8;++i){const unsigned at=64+i*32;auto& e=p.parts[i];if(file.be16(at+2))return fail("entry reserved");e.record=file.be16(at);e.root=file.be32(at+4);e.delay=std::bit_cast<s16>(file.be16(at+8));e.sound=std::bit_cast<s16>(file.be16(at+10));e.spin_delta=std::bit_cast<s16>(file.be16(at+12));e.initial_spin=std::bit_cast<s16>(file.be16(at+14));e.height=file.bef32(at+16);e.velocity=file.bef32(at+20);e.acceleration=file.bef32(at+24);e.target=file.bef32(at+28);p.settle[i]=file.bef32(320+i*4);}
    p.loaded=true;if(!startup_prop_profile_valid(p))return fail("entry/resource/numeric domain");*out=p;if(error)error->clear();return true;
}
} // namespace gf
