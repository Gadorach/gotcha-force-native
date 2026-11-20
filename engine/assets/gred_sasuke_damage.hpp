#pragma once
#include <array>
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
enum class GRedSasukeDamageRow:u8 {
    CreditDifficulty,CreditHealth,CreditForce,CreditRank,
    ReceiverDifficulty,ReceiverHealth,ReceiverForce,ReceiverRank,ReceiverTier
};
// Exact GFDA v1 local coefficients. The format says nothing about which
// actors are alive, the current table indices, or an admitted collision.
class GRedSasukeDamageTable {
public:
    static constexpr std::size_t byte_size=600;
    bool load(ByteView bytes){
        ready_=false;
        if(!bytes.data||bytes.size!=byte_size||bytes.be32(0)!=0x47464441u||bytes.be16(4)!=1||
           bytes.be16(6)!=0x0615||bytes.be16(8)!=0x000a||bytes.be16(10)||bytes.be32(12)!=byte_size)return false;
        u32 hash=2166136261u;
        for(std::size_t i=0;i<bytes.size;++i)hash=(hash^bytes.data[i])*16777619u;
        if(hash!=0x253480b2u)return false;
        for(std::size_t i=0;i<values_.size();++i){
            const f32 v=bytes.bef32(16+4*i);
            if(!std::isfinite(v)||v<0||v>16)return false;
            values_[i]=v;
        }
        ready_=true;return true;
    }
    bool ready()const{return ready_;}
    bool factor(GRedSasukeDamageRow row,unsigned index,f32* out)const{
        const unsigned slot=unsigned(row);
        if(!out||!ready_||slot+1>=offsets.size()||index>=unsigned(offsets[slot+1]-offsets[slot]))return false;
        *out=values_[offsets[slot]+index];return true;
    }
private:
    static constexpr std::array<u8,10> offsets{0,3,35,67,71,74,106,138,142,146};
    std::array<f32,offsets.back()> values_{};
    bool ready_=false;
};
}
