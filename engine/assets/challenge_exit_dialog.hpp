#pragma once
#include "engine/assets/challenge_difficulty_art.hpp"
namespace gf {
struct ChallengeExitPart {u16 record=0;std::array<f32,3> position{};};
struct ChallengeExitDialogArt {
    u32 bank_hash=0;std::array<ChallengeExitPart,6> parts;
    f32 depth=0,panel_end=0,close_start=0,choice_step=0;u16 open_wait=0,cancel_wait=0,accept_wait=0;
    bool valid()const{
        if(!bank_hash||!open_wait||open_wait>1024||!cancel_wait||cancel_wait>1024||!accept_wait||accept_wait>1024||!std::isfinite(depth)||std::abs(depth)>1e6f||!std::isfinite(panel_end)||panel_end<1||panel_end>8192||!std::isfinite(close_start)||close_start<1||close_start>panel_end||!std::isfinite(choice_step)||std::abs(choice_step)>1e6f)return false;
        for(const auto& p:parts){if(p.record>=4096)return false;for(f32 v:p.position)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;}
        return true;
    }
    bool load(ByteView bytes,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Challenge exit dialog: profile extent/domain";return false;};
        if(!bytes.data||bytes.size!=132||bytes.be32(0)!=0x47464558||bytes.be32(4)!=1||bytes.be16(130))return fail();
        bank_hash=bytes.be32(8);std::size_t at=12;bool ok=true;
        for(auto& p:parts){const auto id=bytes.be32(at);at+=4;if(id>=4096)ok=false;p.record=u16(id);for(auto& v:p.position){v=bits_to_f32(bytes.be32(at));at+=4;}}
        f32* values[]={&depth,&panel_end,&close_start,&choice_step};for(auto* v:values){*v=bits_to_f32(bytes.be32(at));at+=4;}
        open_wait=bytes.be16(at);cancel_wait=bytes.be16(at+2);accept_wait=bytes.be16(at+4);
        if(!ok||!valid())return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
