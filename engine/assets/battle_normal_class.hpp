#pragma once
#include "engine/assets/battle_character_resources.hpp"
namespace gf {
//Original addresses are provenance keys only; they are NEVER native call targets.
struct BattleNormalClassRow {
    u16 model=0;u8 variant=0;u32 model_hash=0,initializer_key=0,config_key=0;
    std::array<u32,16> fields4b4_to_4f0{};std::array<u32,4> program_table_keys{};
    u32 parameters27c_key=0,config_hash=0,actor_data_hash=0;
};
class BattleNormalClassProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle normal class: extent/model/provenance domain";return false;};
        if(!b.data||b.size<32||b.size>256||b.be32(0)!=0x47464249u||b.be32(4)!=1||!b.be16(8)||b.be16(8)>2||b.be16(10)!=112||b.be32(12)!=b.size-32||b.size!=32+std::size_t(b.be16(8))*112)return fail();
        for(unsigned i=16;i<32;++i)if(b.data[i])return fail();
        auto key=[](u32 k){return !k||(k>=0x80000000u&&k<0x81800000u);};std::vector<BattleNormalClassRow> rows;
        for(unsigned i=0;i<b.be16(8);++i){const unsigned at=32+112*i;BattleNormalClassRow r;r.model=b.be16(at);r.variant=b.data[at+2];r.model_hash=b.be32(at+4);r.initializer_key=b.be32(at+8);r.config_key=b.be32(at+12);
            if((r.model!=0x615&&r.model!=10)||r.variant||b.data[at+3]||!r.initializer_key||!r.config_key||!key(r.initializer_key)||!key(r.config_key)||b.be32(at+108))return fail();
            for(const auto& old:rows)if(old.model==r.model)return fail();
            for(unsigned j=0;j<16;++j){r.fields4b4_to_4f0[j]=b.be32(at+16+4*j);if(!key(r.fields4b4_to_4f0[j]))return fail();}
            for(unsigned j=0;j<4;++j){r.program_table_keys[j]=b.be32(at+80+4*j);if(!r.program_table_keys[j]||!key(r.program_table_keys[j]))return fail();}
            r.parameters27c_key=b.be32(at+96);r.config_hash=b.be32(at+100);r.actor_data_hash=b.be32(at+104);if(!r.fields4b4_to_4f0[1]||!r.parameters27c_key||!key(r.parameters27c_key))return fail();
            rows.push_back(r);
        }
        rows_=std::move(rows);if(error)error->clear();return true;
    }
    const BattleNormalClassRow* find(u16 model,u8 variant)const{for(const auto& r:rows_)if(r.model==model&&r.variant==variant)return &r;return nullptr;}
    bool ready()const{return !rows_.empty();}
private:std::vector<BattleNormalClassRow> rows_;
};
} // namespace gf
