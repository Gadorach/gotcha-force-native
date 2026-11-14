#pragma once
#include "engine/animation/linear_key_curve.hpp"
#include "engine/assets/stih.hpp"
namespace gf {
struct StartupLandingEntry {u16 record=0,duration=0;std::vector<LinearKey<3>> scale;std::vector<LinearKey<1>> alpha;};
struct StartupLandingProfile {
    u32 model_hash=0;StageGroundParameters ground;f32 rejection_fraction=0,one=0,angle_unit=0;std::array<StartupLandingEntry,3> entries{};bool loaded=false;
};
inline bool startup_landing_profile_valid(const StartupLandingProfile& p){
    if(!p.loaded||!p.model_hash||!stage_ground_parameters_valid(p.ground)||f32_to_bits(p.rejection_fraction)!=0x3f000000||f32_to_bits(p.one)!=0x3f800000||f32_to_bits(p.angle_unit)!=0x38c90fdb)return false;
    const std::array<u16,3> records={31,32,5},durations={10,15,25};
    for(unsigned i=0;i<3;++i){const auto& e=p.entries[i];if(e.record!=records[i]||e.duration!=durations[i]||e.scale.empty()||e.scale.size()>128||e.alpha.size()>128)return false;
        auto valid=[](const auto& keys){for(unsigned n=0;n<keys.size();++n){if(keys[n].time<0||keys[n].time>8192||(n&&keys[n].time<=keys[n-1].time))return false;for(auto v:keys[n].value)if(!std::isfinite(v)||std::abs(v)>1e4f)return false;}return true;};
        if(!valid(e.scale)||!valid(e.alpha))return false;
    }return true;
}
inline bool startup_landing_load(ByteView b,StartupLandingProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup landing profile: ")+why;return false;};
    if(!out||!b.data||b.size<104||b.size>16384||b.be32(0)!=0x47464c44||b.be16(4)!=1||b.be16(6)!=3||b.be32(8)!=b.size)return fail("header/extent");
    StartupLandingProfile p;p.loaded=true;p.model_hash=b.be32(12);auto& g=p.ground;g.declared=true;g.bounds_min=b.bef32(16);g.bounds_max=b.bef32(20);g.cell_size=b.bef32(24);g.zero=b.bef32(28);g.secondary_normal_limit=b.bef32(32);g.high34=b.bef32(36);g.low68=b.bef32(40);p.rejection_fraction=b.bef32(44);p.one=b.bef32(48);p.angle_unit=b.bef32(52);
    std::size_t cursor=104;
    for(unsigned i=0;i<3;++i){const auto at=56+i*16;auto& e=p.entries[i];e.record=b.be16(at);e.duration=b.be16(at+2);const u16 scales=b.be16(at+4),alphas=b.be16(at+6);
        if(scales>128||alphas>128||b.be32(at+8)!=cursor||!b.contains(cursor,std::size_t(scales)*16))return fail("scale curve extent");
        for(unsigned n=0;n<scales;++n){LinearKey<3> k;k.time=std::bit_cast<s32>(b.be32(cursor));for(unsigned c=0;c<3;++c)k.value[c]=b.bef32(cursor+4+c*4);e.scale.push_back(k);cursor+=16;}
        if(b.be32(at+12)!=cursor||!b.contains(cursor,std::size_t(alphas)*8))return fail("alpha curve extent");
        for(unsigned n=0;n<alphas;++n){LinearKey<1> k;k.time=std::bit_cast<s32>(b.be32(cursor));k.value[0]=b.bef32(cursor+4);e.alpha.push_back(k);cursor+=8;}
    }
    if(cursor!=b.size||!startup_landing_profile_valid(p))return fail("curve/operand domain");
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
