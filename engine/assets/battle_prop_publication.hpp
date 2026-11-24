#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//89EB4's initial48-prop prefix only. Side/child callbacks and presentation are
//not skipped or acknowledged by this receipt.
class BattlePropPublicationProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="prop publication: initial source group0 callback recipe required";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x47465050||b.be32(4)!=1||b.be32(8)!=80||b.be32(12))return fail();
        const std::array<u32,16> words{0x80089eb4,0x80087b64,0x80087c84,0x8008b290,0x800097b4,0x8008c0dc,0x8008c1b0,0x80009790,48,30,18,0x17,0x14,0x0b,1,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
