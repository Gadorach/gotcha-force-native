#pragma once
#include <vector>
#include <string>
#include <cmath>
#include "engine/core/types.hpp"
#include "engine/assets/battle_model_domain.hpp"
namespace gf {
struct BattleActorShapeSeed {u8 field89=0,type11=0,argument12=0;f32 scale_b4=0;};
//1C2C8's qualified model→type0 tables, exported through their negative sentinel.
//This profile never supplies allocation outcomes or callback completion.
class BattleActorEffectsProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle actor effects: source table/model/extent";return false;};
        if(!b.data||b.size<32||b.size>32+16*12||b.be32(0)!=0x47465345||(b.be32(4)!=1&&b.be32(4)!=2)||b.be32(8)!=b.size||!b.be32(12)||!battle_model_in_domain(BattleModelDomain(b.be32(4)),b.be16(16))||!b.be16(18)||b.be16(18)>16||b.size!=32+12u*b.be16(18)||b.be16(20)!=0||b.be16(22)!=12||b.be32(24)||b.be32(28))return fail();
        std::vector<BattleActorShapeSeed> rows;
        for(unsigned i=0;i<b.be16(18);++i){const unsigned at=32+12*i;const auto scale=b.bef32(at+4);if(b.be16(at)>127||b.be16(at+2)>127||!std::isfinite(scale)||scale<=0||scale>1e4f)return fail();rows.push_back({u8(b.be16(at)),u8(b.be16(at+2)),u8(b.be32(at+8)),scale});}
        model_=b.be16(16);hash_=b.be32(12);rows_=std::move(rows);if(error)error->clear();return true;
    }
    bool for_model(u16 model,u32 hash)const{return !rows_.empty()&&model==model_&&hash==hash_;}
    const std::vector<BattleActorShapeSeed>& shapes()const{return rows_;}
private:u16 model_=0;u32 hash_=0;std::vector<BattleActorShapeSeed> rows_;
};
} // namespace gf
