#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialBarChildProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial bar: exact G-Red/count5/pool3 source recipe required";return false;};
        if(!b.contains(0,96)||b.size!=96||b.be32(0)!=0x4746424b||b.be32(4)!=1||b.be32(8)!=96||b.be32(12))return fail();
        const std::array<u32,12> keys{0x8013be88,0x8013bec4,0x80006dc8,0x80197a0c,0x8025afa4,0x80197ad8,0x80006f1c,0x80089100,0x8013c144,0x8013c264,0x80029434,0x80089898};
        const std::array<u32,8> words{0x615,3,5,0x24,180,192,1,3};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(64+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
