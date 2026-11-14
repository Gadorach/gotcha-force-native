#pragma once
#include <array>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct StartupSoundCommandProfile {u32 tone_hash=0;std::array<f32,5> gains{};bool loaded=false;};
inline bool startup_sound_command_profile_valid(const StartupSoundCommandProfile& p){
    constexpr std::array<u32,5> expected={0x3f19999a,0x3f333333,0x3f4ccccd,0x3f666666,0x3f800000};
    if(!p.loaded||!p.tone_hash)return false;
    for(unsigned i=0;i<5;++i)if(f32_to_bits(p.gains[i])!=expected[i])return false;
    return true;
}
inline bool startup_sound_command_load(ByteView b,StartupSoundCommandProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound command profile: ")+why;return false;};
    if(!out||!b.contains(0,48)||b.size!=48||b.be32(0)!=0x47465343||b.be16(4)!=1||b.be16(6)!=5||b.be32(8)!=48||b.be32(12)||b.be32(40)||b.be32(44))return fail("header/extent/reserved");
    StartupSoundCommandProfile p;p.loaded=true;p.tone_hash=b.be32(16);for(unsigned i=0;i<5;++i)p.gains[i]=b.bef32(20+4*i);
    if(!startup_sound_command_profile_valid(p))return fail("tone binding/source gains");
    *out=p;if(error)error->clear();return true;
}
struct StartupSoundToneBank {std::vector<std::array<u8,16>> tones;u32 hash=0;bool loaded=false;};
namespace startup_sound_commands_asset_detail {
inline u32 fnv(ByteView b){u32 value=2166136261u;for(std::size_t i=0;i<b.size;++i)value=(value^b.data[i])*16777619u;return value;}
inline u32 canonical_hash(const StartupSoundToneBank& b){
    std::array<u8,16> header={0x54,0x53,0x42,0x44,0,0,0,0,0,1,0,0,0,0,0,0};const u32 size=u32(16+b.tones.size()*16),count=u32(b.tones.size());
    for(unsigned i=0;i<4;++i){header[4+i]=u8(size>>(8*i));header[12+i]=u8(count>>(8*i));}
    u32 value=fnv({header.data(),header.size()});for(const auto& row:b.tones)for(auto x:row)value=(value^x)*16777619u;return value;
}
}
inline bool startup_sound_tones_valid(const StartupSoundCommandProfile& p,const StartupSoundToneBank& b){
    return startup_sound_command_profile_valid(p)&&b.loaded&&!b.tones.empty()&&b.tones.size()<=128&&b.hash==p.tone_hash&&startup_sound_commands_asset_detail::canonical_hash(b)==b.hash;
}
inline bool startup_sound_tones_load(ByteView b,const StartupSoundCommandProfile& p,StartupSoundToneBank* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound tones: ")+why;return false;};
    if(!out||!startup_sound_command_profile_valid(p)||!b.contains(0,16)||b.size>2064)return fail("profile/header/budget");
    auto le=[&](unsigned at){return u32(b.data[at])|(u32(b.data[at+1])<<8)|(u32(b.data[at+2])<<16)|(u32(b.data[at+3])<<24);};
    const u32 count=le(12);
    if(b.be32(0)!=0x54534244||le(4)!=b.size||le(8)!=256||!count||count>128||b.size!=16+count*16||startup_sound_commands_asset_detail::fnv(b)!=p.tone_hash)return fail("TSBD header/extent/FNV binding");
    StartupSoundToneBank next;next.loaded=true;next.hash=p.tone_hash;next.tones.resize(count);
    for(unsigned i=0;i<count;++i)for(unsigned j=0;j<16;++j)next.tones[i][j]=b.data[16+i*16+j];
    *out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
