#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
//97854 profile1/pool1:32-byte descriptor records,06E3C/06E8C archive hooks.
//Counts describe the fresh recipe, not current scene occupancy or68-byte ABI.
class SceneArchivePoolProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};if(!b.data||b.size!=40||b.be32(0)!=0x47464150||b.be32(4)!=1||b.be32(8)!=40||b.be32(12)||b.be32(16)!=0x1ee800||!b.be32(20)||b.be32(20)/32==0||b.be32(20)/32>32767||b.be16(24)!=32||b.data[26]!=1||b.data[27]!=1||b.be32(28)!=0x80006e3c||b.be32(32)!=0x80006e8c||b.be32(36)!=68){if(error)*error="scene archive pool: pool1/fresh recipe domain";return false;}
        bytes_=b.be32(20);ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u16 capacity()const{return ready_?u16(bytes_/32):0;}u32 unused_bytes()const{return ready_?bytes_%32:0;}
private:u32 bytes_=0;bool ready_=false;
};
} // namespace gf
