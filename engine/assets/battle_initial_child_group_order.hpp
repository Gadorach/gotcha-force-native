#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialChildGroupOrderProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial child groups: exact29434 group0/10/8/18 order required";return false;};
        if(!b.contains(0,64)||b.size!=64||b.be32(0)!=0x4746434f||b.be32(4)!=1||b.be32(8)!=64||b.be32(12))return fail();
        const std::array<u32,12> words{0x80029434,0x80089a94,0x8008997c,0x80089898,0,0x10,8,0x18,6,0x38,0,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
