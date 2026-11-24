#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialShapeWorldProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="shape world: exact first static type0 projection recipe required";return false;};
        const std::array<u32,32> w{0x47465357,1,128,0,0x8001c5ac,0x8003f688,0x8003e0a8,0x80045ef4,0x800462e4,0x8020b4f8,0x8020af5c,0x8020bc88,0x80055dcc,0x8003cc24,0x80089554,0x802d0d44,0,4,0x2edbe6fe,0xbdf8e542,0,0x3f800000,0x40000000,0x42c80000,0xc59c4000,0x80437080,0,1,0,19,16,0};
        if(!b.contains(0,128)||b.size!=128)return fail();
        for(unsigned i=0;i<w.size();++i)if(b.be32(4*i)!=w[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
