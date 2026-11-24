#pragma once
#include "engine/assets/challenge_finalization.hpp"
namespace gf {
//40F1C/40F74: common PZZ member696, four independently bounded native blocks.
//Source offsets remain provenance only, not host pointers or an emulated arena.
class BattleCommonResourceProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle common resources: four-block recipe/extent";return false;};
        if(!b.data||b.size!=64||b.be32(0)!=0x4746434f||b.be32(4)!=1||b.be32(8)!=64||b.be32(12)!=696||b.be32(16)<48||b.be32(16)>(32u<<20)||!b.be32(20)||b.be32(40)!=0x25f400||b.be32(44)!=0x264c00||b.be32(48)!=0x1f3400||b.be32(52)!=0x1f2c00||b.be32(56)!=0xa49800||b.be32(60))return fail();
        u64 sum=48;for(unsigned i=0;i<4;++i){sizes_[i]=b.be32(24+4*i);if(!sizes_[i]||sizes_[i]>(16u<<20))return fail();sum+=sizes_[i];}if(sum!=b.be32(16))return fail();bytes_=u32(sum);hash_=b.be32(20);ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 bytes()const{return bytes_;}u32 hash()const{return hash_;}u32 block_bytes(unsigned i)const{return ready_&&i<4?sizes_[i]:0;}
private:std::array<u32,4> sizes_{};u32 bytes_=0,hash_=0;bool ready_=false;
};
} // namespace gf
