#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
//54070's first phase0 call, including its5685C prelude. Not a ticket to resume
//an arbitrary constructed body at465FC or to execute later phase1 updates.
class BattleActorFirstUpdateProfile {
public:
    bool load(ByteView bytes,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="battle first update profile: exact fresh caller recipe";return false;};
        if(!bytes.contains(0,64)||bytes.size!=64||bytes.be32(0)!=0x47464655||bytes.be32(4)!=1||bytes.be32(8)!=64||bytes.be32(12)||bytes.be32(48)!=0x41a00000||bytes.be32(52)||bytes.be32(56)||bytes.be32(60))return fail();
        const std::array<u32,8> keys{0x80054070,0x8005685c,0x802d3264,0x800541ac,0x8008b480,0x8005a580,0x800465fc,0x80006284};
        for(unsigned i=0;i<keys.size();++i)if(bytes.be32(16+4*i)!=keys[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}f32 bound_margin()const{return 20;}
private:bool ready_=false;
};
}
