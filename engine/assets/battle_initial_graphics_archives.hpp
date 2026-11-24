#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Only425B8/42954 archive bindings and retained08B90 constructor inputs.
//No42300 relocation, point-light shader or enclosing286D8 acknowledgement.
class BattleInitialGraphicsArchivesProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial graphics archives: exact split-mode source fragment required";return false;};
        if(!b.contains(0,96)||b.size!=96||b.be32(0)!=0x47464741||b.be32(4)!=1||b.be32(8)!=96||b.be32(12))return fail();
        const std::array<u32,12> keys{0x800286d8,0x800425b8,0x80042954,0x80197a0c,0x80040eb4,0x80006c88,0x80008b90,0x80008c40,0x80245f20,0x80043414,0x80042300,0x8004be08};
        const std::array<u32,8> words{1,2708,0x2fbc80c9,2,0x6dd5d3b2,118,157,4};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(64+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
