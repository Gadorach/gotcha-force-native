#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//8B650's MAIN held/previous/pressed/released fields, restricted to the digital
//1F70 mask. This does not admit analogue direction production or repeat words.
class SourceMainDigitalInputProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="MAIN digital input: exact pressed-word producer/subset required";return false;};
        if(!b.contains(0,64)||b.size!=64)return fail();
        const std::array<u32,16> words{0x47464d49,1,64,0,0x8008b650,0x8008b7c8,0x803c727c,0x803c71fc,4,32,8,12,16,0x1f70,0,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
