#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//42878: member697, texture block0 and mode-selected model block1/2;
//8BED8's reached one-image texture/palette destination capacities.
class BattleEffectResourceProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle effect profile: source recipe/extent";return false;};
        if(!b.data||b.size!=64||b.be32(0)!=0x47464550||b.be32(4)!=1||b.be32(8)!=64||b.be32(12)!=697||b.be32(16)<40||b.be32(16)>(32u<<20)||!b.be32(20)||b.be32(36)!=0x7a3c00||b.be32(40)!=0x7ac400||b.be32(44)!=0x8008bed8||b.be32(48)!=4||b.be32(52)!=1||b.be32(56)||b.be32(60))return fail();
        u64 total=40;for(unsigned i=0;i<3;++i){sizes_[i]=b.be32(24+4*i);if(sizes_[i]<32||sizes_[i]>(16u<<20))return fail();total+=sizes_[i];}
        if(total!=b.be32(16))return fail();
        bytes_=u32(total);hash_=b.be32(20);ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 bytes()const{return bytes_;}u32 hash()const{return hash_;}
    u32 block_bytes(unsigned i)const{return ready_&&i<3?sizes_[i]:0;}
private:std::array<u32,3> sizes_{};u32 bytes_=0,hash_=0;bool ready_=false;
};
} // namespace gf
