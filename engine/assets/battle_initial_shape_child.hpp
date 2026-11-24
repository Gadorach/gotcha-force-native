#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialShapeChildProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial shape child: exact070B bone19/stage2 constructor recipe required";return false;};
        if(!b.contains(0,116)||b.size!=116||b.be32(0)!=0x47465353||b.be32(4)!=1||b.be32(8)!=116||b.be32(12))return fail();
        const std::array<u32,12> keys{0x8001c42c,0x8001c468,0x8001c524,0x80089100,0x80008a94,0x80245f20,0x80040e94,0x802442dc,0x80245a8c,0x80245acc,0x8020bab8,0x8020bb14};
        const std::array<u32,13> words{0x070b,0xbdb1d4f4,0xbdf8e542,2,0,0,19,0x3f800000,0x3a83126f,0x38100000,0,0x10,1};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(64+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
