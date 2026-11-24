#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//87814's first group0 callback, before the object/actor callbacks. Only the
//scene45=1, sceneD9=0, camera-invariant normal-root branch is admitted.
class BattleStagePublicationProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="stage publication: initial source callback recipe required";return false;};
        if(!b.contains(0,96)||b.size!=96||b.be32(0)!=0x47465355||b.be32(4)!=1||b.be32(8)!=96||b.be32(12))return fail();
        const std::array<u32,20> words{0x80087814,0x80087f38,0x8008c0dc,0x8008c1b0,0x8008cec8,0x8008c440,0x8008b290,0x800097b4,0x80009790,0x80087f20,0,1,0,0x58,0x64,0x0f,0x0b,1,0,2};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
