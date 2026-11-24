#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Initial070B slot0/row3 field follower only. No group/frame completion.
class BattleInitialFieldChildProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial field child: exact fresh070B row3 source recipe required";return false;};
        if(!b.contains(0,128)||b.size!=128||b.be32(0)!=0x47464649||b.be32(4)!=1||b.be32(8)!=128||b.be32(12))return fail();
        const std::array<u32,12> keys{0x80089a94,0x8008997c,0x800c6394,0x800660e8,0x800c63e8,0x800c6544,0x80046588,0x80045204,0x80089100,0x80007030,0x80007c54,0x80007cac};
        const std::array<u32,16> words{0x803052fc,0x0001000c,0,0x41f00000,0,0,0x0000ffff,0,0,0x0000ffff,0x3ecccccd,0x070b,0x1e,0x41,1,0};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(64+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
