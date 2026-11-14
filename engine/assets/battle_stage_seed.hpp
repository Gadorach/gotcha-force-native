#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
struct BattleStageSeedRow {u32 stage=0,high_bits=0,low_bits=0;};
class BattleStageSeedProfile {
public:
    bool load(ByteView b,std::string* error){ready_=false;auto fail=[&](){if(error)*error="stage seed: normal Challenge source scalar domain";return false;};if(!b.data||b.size!=164||b.be32(0)!=0x47465353||b.be32(4)!=1||b.be32(8)!=164||b.be32(12)!=0x100||b.be32(16)||b.be32(20)!=0x459c4000||b.be32(24)!=0xc59c4000||b.be32(28)!=0x43480000)return fail();const std::array<u32,11> ids{0,1,2,3,4,5,8,10,11,12,14};for(unsigned i=0;i<11;++i){const unsigned at=32+12*i;const u32 low=ids[i]==2?0xc25c0000:ids[i]==8?0x42dc0000:0xc1200000;if(b.be32(at)!=ids[i]||b.be32(at+4)!=0x459c4000||b.be32(at+8)!=low)return fail();rows_[i]={ids[i],b.be32(at+4),b.be32(at+8)};}ready_=true;if(error)error->clear();return true;}
    const BattleStageSeedRow* find(u32 stage)const{if(ready_)for(const auto& r:rows_)if(r.stage==stage)return &r;return nullptr;}bool ready()const{return ready_;}
private:std::array<BattleStageSeedRow,11> rows_{};bool ready_=false;
};
} // namespace gf
