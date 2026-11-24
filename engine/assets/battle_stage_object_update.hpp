#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//First group40 update and the initial two89554 side-world callbacks only.
class BattleStageObjectUpdateProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="stage object update: source initial callback recipe required";return false;};
        if(!b.contains(0,96)||b.size!=96||b.be32(0)!=0x47465357||b.be32(4)!=1||b.be32(8)!=96||b.be32(12))return fail();
        const std::array<u32,20> words{0x80089a94,0x8008997c,0x80089898,0x80087b58,0x80081d98,0x80081dd4,0x80089554,0x80082060,0x80047aa4,0x80089cd0,0x80089d28,40,1,0x82,0x1ca,0x2c,0x76,0x0f,0,2};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
