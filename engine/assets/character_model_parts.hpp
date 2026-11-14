#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
struct CharacterModelPartGroup {u8 base=0,bone=0,count=0;};
struct CharacterModelPartsProfile {u32 model_hash=0;u16 model_id=0;std::array<CharacterModelPartGroup,2> groups{};bool loaded=false;};
inline bool character_model_parts_valid(const CharacterModelPartsProfile& p){
    return p.loaded&&p.model_hash&&(p.model_id==0x615||p.model_id==10)&&p.groups[0].base==1&&p.groups[0].bone==7&&p.groups[0].count==5&&p.groups[1].base==6&&p.groups[1].bone==11&&p.groups[1].count==(p.model_id==0x615?3:5);
}
// Source4FCFC/5380C table rows; counts are initialized per logical bone by5380C.
// This bounded profile is only the two normal models, not inferred all-model art.
inline bool character_model_parts_load(ByteView bytes,u32 model_hash,CharacterModelPartsProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="character model parts: source table/model/header";return false;};
    if(!out||!bytes.contains(0,24)||bytes.be32(0)!=0x47465054u||bytes.be16(4)!=1||bytes.be32(8)!=model_hash||bytes.be32(12)!=bytes.size||bytes.data[18]!=2||bytes.data[19]||bytes.be32(20))return fail();
    const unsigned count=bytes.be16(16);if(!count||count>32||bytes.size!=24+count*4)return fail();CharacterModelPartsProfile p;p.model_hash=model_hash;p.model_id=bytes.be16(6);std::array<bool,2> seen{};
    for(unsigned i=0;i<count;++i){const auto* row=bytes.data+24+i*4;const s8 bone=std::bit_cast<s8>(row[1]),kind=std::bit_cast<s8>(row[2]);if(row[0]!=i+1||row[3]||(bone!=7&&bone!=11)||kind< -1||kind>1)return fail();const unsigned group=bone==7?0:1;
        if(kind>=0){if(unsigned(kind)!=group||seen[group])return fail();seen[group]=true;p.groups[group].base=row[0];p.groups[group].bone=u8(bone);}
        else if(!seen[group])return fail();
        ++p.groups[group].count;if(p.groups[group].base+p.groups[group].count!=row[0]+1)return fail();
    }
    p.loaded=true;if(!seen[0]||!seen[1]||!character_model_parts_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
struct CharacterModelPartSelection {std::array<u16,2> record{};std::array<bool,2> visible{};};
inline bool character_model_parts_select(const CharacterModelPartsProfile& p,std::array<u8,2> selectors,u32 visible,u32 suppressed,CharacterModelPartSelection* out){
    if(out)*out={};
    if(!out||!character_model_parts_valid(p)||selectors[0]>=128||selectors[1]>=128)return false;
    CharacterModelPartSelection result;
    for(unsigned i=0;i<2;++i){const auto& g=p.groups[i];if(!(visible&(u32(1)<<g.bone))||(suppressed&(u32(1)<<g.bone))||selectors[i]>=g.count)continue;result.record[i]=u16(g.base+selectors[i]);result.visible[i]=true;}
    *out=result;return true;
}
} //namespace gf
