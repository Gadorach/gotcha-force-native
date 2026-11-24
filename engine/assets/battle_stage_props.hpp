#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Stage2's87924 ranges/selectors and87D2C root operations. Later updates,
//visibility, collision registration and draw callbacks are separate consumers.
class BattleStagePropsProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="stage props profile: source stage2 constructor operands required";return false;};
        if(!b.contains(0,128)||b.size!=128||b.be32(0)!=0x47465350||b.be32(4)!=1||b.be32(8)!=128||b.be32(12))return fail();
        const std::array<u32,12> words{2,0x80087924,0x8008784c,0x80087b58,0x80087b64,0x80087c84,0x80087d2c,0x800070e4,0x4622f983,0x3f000000,0x3f800000,0x47435000};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        if(b.be32(64)!=0x03202132)return fail();
        const std::array<u8,60> selectors{41,255,42,255,43,255,44,255,43,255,45,255,255,255,255,255,255,255,50,255,51,255,255,255,52,255,255,255,54,255,255,255,255,255,255,255,255,255,255,255,56,255,57,255,58,255,255,255,255,255,53,255,53,255,55,255,55,255,255,255};
        for(unsigned i=0;i<selectors.size();++i)if(b.data[68+i]!=selectors[i])return fail();
        selectors_=selectors;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    std::array<u8,2> selectors(unsigned record)const{return ready_&&record>=3&&record<=32?std::array<u8,2>{selectors_[2*(record-3)],selectors_[2*(record-3)+1]}:std::array<u8,2>{255,255};}
private:std::array<u8,60> selectors_{};bool ready_=false;
};
}
