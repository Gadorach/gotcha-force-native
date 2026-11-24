#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialUiWorldProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial UI worlds: exact initial visibility/briefing/HUD callback recipe required";return false;};
        const std::array<u32,44> words{0x47465557,1,176,0,0x80036e24,0x8008972c,0x80089658,0x80047aa4,0x80080f88,0x800818cc,0x8006ae60,0x800537a8,0x80029434,0x8002945c,0x80029488,0x7d2,1,1,13,38,8,0x20,0x21,1,0x11,0x19,0x64,0x58,0x74,0x38c90fdb,0x1e00,2,57,82,62,0x2fbc80c9,0x22e678b4,0x87d2f859,4,5,0x80053a94,0x80089898,59,0x8000bda4};
        if(!b.contains(0,176)||b.size!=176)return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
