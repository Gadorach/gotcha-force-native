#pragma once
#include "engine/assets/force_catalog.hpp"
#include "engine/assets/battle_model_domain.hpp"
namespace gf {
struct BattleActorLevelRow {u16 model=0;u8 variant=0,level=0;u32 model_hash=0;s16 full_cost=0;std::array<s16,9> values{};BattleModelDomain domain=BattleModelDomain::NormalPair;};
//GFSL1 normal-pair / GFSL2 initial-AI canonical-level rows. Neither expands the
//old level0-only GFBR API or imports an actor memory image.
class BattleActorLevelProfile {
public:
    bool load(ByteView bytes,const ForceCatalog& catalog,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle actor levels: source model/catalog/level-row extent";return false;};
        if(!catalog.ready()||!bytes.data||bytes.size<32||bytes.size>424||bytes.be32(0)!=0x4746534cu||(bytes.be32(4)!=1&&bytes.be32(4)!=2)||bytes.be32(8)!=bytes.size||!bytes.be16(12)||bytes.be16(12)>2||bytes.be16(14)!=196||bytes.size!=32+std::size_t(bytes.be16(12))*196||bytes.be32(16)!=0x3f2aaaabu||bytes.be32(20)||bytes.be32(24)||bytes.be32(28))return fail();
        std::vector<BattleActorLevelRow> rows;
        for(unsigned i=0;i<bytes.be16(12);++i){const unsigned at=32+196*i;const u16 model=bytes.be16(at);const auto* source=catalog.find(model);const s16 cost=std::bit_cast<s16>(bytes.be16(at+10));
            if(!battle_model_in_domain(BattleModelDomain(bytes.be32(4)),model)||!source||bytes.data[at+2]||bytes.data[at+3]!=10||!bytes.be32(at+4)||bytes.data[at+8]!=2||bytes.data[at+9]||cost<0||cost!=source->cost[0]||bytes.be32(at+12))return fail();
            for(const auto& old:rows)if(old.model==model)return fail();
            for(unsigned level=0;level<10;++level){BattleActorLevelRow row;row.domain=BattleModelDomain(bytes.be32(4));row.model=model;row.level=u8(level);row.model_hash=bytes.be32(at+4);row.full_cost=cost;for(unsigned k=0;k<9;++k)row.values[k]=std::bit_cast<s16>(bytes.be16(at+16+level*18+k*2));if(row.values[0]<0)return fail();rows.push_back(row);}
        }
        rows_=std::move(rows);catalog_=catalog.fingerprint();loaded_=true;if(error)error->clear();return true;
    }
    bool for_catalog(const ForceCatalog& catalog)const{return loaded_&&catalog.ready()&&catalog.fingerprint()==catalog_;}
    const BattleActorLevelRow* find(u16 model,u8 variant,u8 level,u32 model_hash)const{if(!loaded_||variant||level>=10)return nullptr;for(const auto& row:rows_)if(row.model==model&&row.level==level&&row.model_hash==model_hash)return &row;return nullptr;}
    bool ready()const{return loaded_;}
private:std::vector<BattleActorLevelRow> rows_;u32 catalog_=0;bool loaded_=false;
};
} // namespace gf
