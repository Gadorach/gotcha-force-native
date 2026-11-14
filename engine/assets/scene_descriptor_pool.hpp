#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
//97854 profile1/pool0 immutable allocation recipe. Hook addresses are DATA keys,
//not host-callable functions. No other pool, caller or global capacity is inferred.
class SceneDescriptorPoolProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="scene descriptor pool: extent/recipe/hook domain";return false;};
        if(!b.data||b.size!=40||b.be32(0)!=0x47464450||b.be32(4)!=1||b.be32(8)!=40||b.be32(12)||b.be16(24)!=44||b.data[26]||b.data[27]!=1||b.be32(28)!=0x80006f60||b.be32(32)!=0x80006ed8||b.be32(36))return fail();
        const u32 offset=b.be32(16),bytes=b.be32(20);if(!offset||!bytes||bytes/44==0||bytes/44>32767||u64(offset)+bytes>0xffffffffu)return fail();
        offset_=offset;bytes_=bytes;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 source_offset()const{return offset_;}u32 bytes()const{return bytes_;}u16 capacity()const{return ready_?u16(bytes_/44):0;}u32 unused_bytes()const{return ready_?bytes_%44:0;}
private:u32 offset_=0,bytes_=0;bool ready_=false;
};
} // namespace gf
