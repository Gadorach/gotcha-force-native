#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <algorithm>
#include <string>
namespace gf {
//88398 object profile0, physical pool2/class2 ONLY. Metadata is not a current
//battle/registry receipt. Other pool classes and profile selectors stay separate.
class SceneObjectPoolProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};if(!b.data||b.size!=96||b.be32(0)!=0x47464f50||b.be32(4)!=1||b.be32(8)!=96||b.be32(12)||!b.be16(16)||b.be16(16)>4096||b.be16(18)!=452||b.data[20]!=2||b.data[21]!=2||b.be16(22)||b.be32(24)||b.be32(28)){if(error)*error="scene object pool: class2/profile0 extent/recipe";return false;}
        count_=b.be16(16);std::copy_n(b.data+32,64,kinds_.begin());ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u16 capacity()const{return count_;}u8 kind(unsigned group)const{return group<64?kinds_[group]:255;}
private:std::array<u8,64> kinds_{};u16 count_=0;bool ready_=false;
};
} // namespace gf
