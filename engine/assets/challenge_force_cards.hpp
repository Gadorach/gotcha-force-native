#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
// Typed type10 card operands only. String/assertion words interleaved with the
// source scalar pool are deliberately not part of this data format.
struct ChallengeForceCardsArt {
    struct Format {u16 record=0;s16 angle=0;};
    u32 bank_hash=0;
    std::array<Format,4> formats{};
    std::array<std::array<f32,2>,9> single_row{};
    std::array<f32,3> single_z{};
    f32 scale_step=0,scale_min=0,scale_max=0,y=0,z_scale=0;
    f32 column_step=0,row_span=0,row_round=0,x_even=0,x_odd=0,z_top=0,z_step=0,grid_scale=0;
    f32 row_colour_step=0,rejected_alpha=0,child_z=0;
    bool valid()const{
        if(!bank_hash)return false;
        for(unsigned i=0;i<formats.size();++i){if(formats[i].record>=4096||formats[i].angle<0||formats[i].angle>16384)return false;for(unsigned j=0;j<i;++j)if(formats[j].record==formats[i].record)return false;}
        if(formats[3].angle)return false;
        for(const auto& row:single_row)for(f32 value:row)if(!std::isfinite(value)||std::abs(value)>1e6f)return false;
        for(f32 value:values())if(!std::isfinite(value)||std::abs(value)>1e6f)return false;
        return scale_step>=0&&scale_min>0&&scale_max>=scale_min&&z_scale>0&&column_step>0&&row_span>0&&row_round>=0&&row_round<1&&z_step>0&&grid_scale>0&&row_colour_step>=0&&row_colour_step*3<256&&rejected_alpha>=0&&rejected_alpha<=1;
    }
    std::array<f32,19> values()const{return {single_z[0],single_z[1],single_z[2],scale_step,scale_min,scale_max,y,z_scale,column_step,row_span,row_round,x_even,x_odd,z_top,z_step,grid_scale,row_colour_step,rejected_alpha,child_z};}
};
inline bool challenge_force_cards_load(ByteView bytes,ChallengeForceCardsArt* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Challenge force cards: GFFC data/domain mismatch";return false;};
    if(!out||!bytes.data||bytes.size!=176||bytes.be32(0)!=0x47464643u||bytes.be32(4)!=1)return fail();
    ChallengeForceCardsArt next;next.bank_hash=bytes.be32(8);std::size_t at=12;
    for(auto& format:next.formats){format.record=bytes.be16(at);format.angle=std::bit_cast<s16>(bytes.be16(at+2));at+=4;}
    auto read=[&](){const f32 value=bytes.bef32(at);at+=4;return value;};
    for(auto& row:next.single_row)for(auto& value:row)value=read();
    for(auto& value:next.single_z)value=read();
    for(auto* value:{&next.scale_step,&next.scale_min,&next.scale_max,&next.y,&next.z_scale,&next.column_step,&next.row_span,&next.row_round,&next.x_even,&next.x_odd,&next.z_top,&next.z_step,&next.grid_scale,&next.row_colour_step,&next.rejected_alpha,&next.child_z})*value=read();
    if(!next.valid())return fail();
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
