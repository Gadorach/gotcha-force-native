#pragma once
#include "engine/assets/challenge_difficulty_art.hpp"
namespace gf {
struct ChallengePlayerPart {s16 record=-1;u16 mask=0;std::array<f32,3> position{};};
struct ChallengePlayerFocus {u16 record=0;std::array<f32,3> position{};};
struct ChallengeBoxPart {u16 record=0,mask=0;f32 scale=1;std::array<f32,3> position{};};
struct ChallengeSetupWidgetsArt {
    u32 bank_hash=0;std::array<std::array<ChallengePlayerPart,4>,2> players;
    std::array<ChallengePlayerFocus,2> focus;std::array<ChallengeBoxPart,2> box;
    //8043B200..B238: player targets/opacity/pulse, focus targets, box targets/period.
    std::array<f32,15> operands{};
    bool valid()const{
        if(!bank_hash)return false;
        for(f32 v:operands)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;
        if(operands[3]<=0||operands[3]>1||operands[4]<0||operands[4]>100||operands[5]<0||operands[5]>1000||operands[6]>0||operands[14]<1||operands[14]>8192)return false;
        for(const auto& group:players)for(const auto& p:group){if(p.record<-1||p.record>=4096||(p.mask&~0x7fu))return false;for(f32 v:p.position)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;}
        for(const auto& p:focus){if(p.record>=4096)return false;for(f32 v:p.position)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;}
        for(const auto& p:box){if(p.record>=4096||(p.mask&~0x7fu)||!std::isfinite(p.scale)||p.scale<=0||p.scale>100)return false;for(f32 v:p.position)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;}
        return true;
    }
    bool load(ByteView bytes,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Challenge player/box art: extent/domain";return false;};
        if(!bytes.data||bytes.size!=272||bytes.be32(0)!=0x47465357||bytes.be32(4)!=1)return fail();
        bank_hash=bytes.be32(8);std::size_t at=12;bool ok=true;
        auto half=[&](){auto v=bytes.be16(at);at+=2;return v;};auto word=[&](){auto v=bytes.be32(at);at+=4;return v;};auto real=[&](){return bits_to_f32(word());};
        for(auto& group:players)for(auto& p:group){p.record=std::bit_cast<s16>(half());p.mask=half();for(auto& v:p.position)v=real();}
        for(auto& p:focus){const auto id=word();if(id>=4096)ok=false;p.record=u16(id);for(auto& v:p.position)v=real();}
        for(auto& p:box){p.record=half();p.mask=half();p.scale=real();for(auto& v:p.position)v=real();}
        for(auto& v:operands)v=real();
        if(!ok||at!=bytes.size||!valid())return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
