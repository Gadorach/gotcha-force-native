#pragma once
#include "engine/assets/source_force_cards.hpp"
namespace gf {
struct SourceForceCardMember {
    std::array<f32,3> position{},scale{};
    u8 row=0,source_rgb=0;
    f32 alpha=0;
};
//80194388 type10 member layout, independent of selection/input and rendering.
//The row partition is 8/7/8/7, distinct from the count16/24 card-format cuts.
inline bool source_force_card_slot(const SourceForceCardsArt& art,unsigned count,unsigned slot,unsigned prefix,SourceForceCardMember* out){
    if(out)*out={};
    if(!out||!art.valid()||count<1||count>30||slot>=30||prefix>30)return false;
    SourceForceCardMember next;next.position[1]=art.y;next.scale[2]=art.z_scale;
    if(count<=8){
        next.position[0]=std::fma(art.single_row[count][1],f32(slot),art.single_row[count][0]);
        next.position[2]=art.single_z[count<4?0:(slot&1)?1:2];
        next.scale[0]=next.scale[1]=std::clamp(std::fma(art.scale_step,f32(8-count),art.scale_min),art.scale_min,art.scale_max);
    }else{
        next.row=u8(unsigned(slot>=8)+unsigned(slot>=15)+unsigned(slot>=23));
        const f32 shift=std::fma(art.row_span,f32(next.row),art.row_round);
        // Guard the profile-derived conversion before touching caller output.
        if(!std::isfinite(shift)||shift<0||shift>1024)return false;
        const f32 column=f32(slot)-f32(unsigned(shift));
        next.position[0]=std::fma(art.column_step,column,-(next.row&1?art.x_odd:art.x_even));
        next.position[2]=std::fma(-art.z_step,f32(next.row),art.z_top);
        next.scale[0]=next.scale[1]=art.grid_scale;
    }
    next.source_rgb=slot<prefix?u8(255-unsigned(art.row_colour_step*next.row)):0;
    next.alpha=slot<prefix?1.f:art.rejected_alpha;
    for(f32 value:next.position)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
    *out=next;return true;
}
//Compact-member compatibility domain; the source slot entry above additionally
//handles stored forces with holes and budget prefixes beyond the member count.
inline bool source_force_card_member(const SourceForceCardsArt& art,unsigned count,unsigned slot,unsigned prefix,SourceForceCardMember* out){
    if(out)*out={};
    return slot<count&&prefix<=count&&source_force_card_slot(art,count,slot,prefix,out);
}
inline bool source_force_card_format(const SourceForceCardsArt& art,unsigned count,bool random,SourceForceCardsArt::Format* out){
    if(out)*out={};
    if(!out||!art.valid()||count>30||(!random&&!count))return false;
    *out=art.formats[random?3:unsigned(count>=16)+unsigned(count>=24)];return true;
}
} // namespace gf
