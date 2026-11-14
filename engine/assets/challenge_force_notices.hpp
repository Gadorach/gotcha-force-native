#pragma once
#include "engine/assets/challenge_force_art.hpp"
namespace gf {
struct ChallengeForceNoticePart {s16 record=-1;std::array<f32,3> position{};};
struct ChallengeForceNoticesArt {
    u32 bank_hash=0;u16 prompt_record=0,initial_tick=0;std::array<f32,3> prompt_position{};
    std::array<ChallengeForceNoticePart,3> team;f32 alpha_step=0;u16 active_blink=0,closing_blink=0,port_record_base=0;
    bool valid()const{
        if(!bank_hash||prompt_record>=4096||initial_tick>32767||team[0].record<0||team[1].record!=-1||team[2].record!=-1||!std::isfinite(alpha_step)||alpha_step<=0||alpha_step>1||!active_blink||!closing_blink||port_record_base>4092)return false;
        for(f32 v:prompt_position)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;
        for(const auto& p:team){if(p.record<-1||p.record>=4096)return false;for(f32 v:p.position)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;}
        return true;
    }
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Challenge force notices: GFNT extent/domain";return false;};
        if(!b.data||b.size!=88||b.be32(0)!=0x47464e54||b.be32(4)!=1)return fail();
        bank_hash=b.be32(8);prompt_record=b.be16(12);initial_tick=b.be16(14);std::size_t at=16;
        auto vector=[&](auto& p){for(auto& v:p){v=b.bef32(at);at+=4;}};vector(prompt_position);
        for(auto& p:team){p.record=std::bit_cast<s16>(b.be16(at));if(b.be16(at+2))return fail();at+=4;vector(p.position);}
        alpha_step=b.bef32(at);at+=4;active_blink=b.be16(at);closing_blink=b.be16(at+2);port_record_base=b.be16(at+4);
        if(b.be16(at+6)||!valid())return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
