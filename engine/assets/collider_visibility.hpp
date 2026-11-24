#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class ColliderVisibilityProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="collider visibility: exact source segment-query operands required";return false;};
        if(!b.contains(0,128)||b.size!=128||b.be32(0)!=0x47464356||b.be32(4)!=1||b.be32(8)!=128||b.be32(12))return fail();
        const std::array<u32,10> keys{0x80036e24,0x80037f20,0x800381d0,0x80038478,0x800389e8,0x80038b34,0x80038c00,0x80039564,0x80039da4,0x802cffb0};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,8> constants{0,0x3f800000,0x3f000000,0x40000000,0x40800000,0x3c23d70a,0x3a83126f,0x80000000};
        for(unsigned i=0;i<constants.size();++i)if(b.be32(56+4*i)!=constants[i])return fail();
        const std::array<u32,10> tail{13,0x800,0x270,0x286,0x26f,1,2,0,1,0};
        for(unsigned i=0;i<tail.size();++i)if(b.be32(88+4*i)!=tail[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
