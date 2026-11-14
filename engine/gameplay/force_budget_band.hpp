#pragma once
#include "engine/assets/force_budget_band.hpp"
#include "engine/assets/force_editor_art.hpp"
#include <algorithm>
#include <optional>
namespace gf {
struct ForceBudgetBandSample {u16 frame=0;s16 angle=0;u8 slot=0;f32 material=0;};
struct ForceBudgetMarkerPose {f32 offset=0;s16 angle=0;};
// Moving boundary position. A full prefix wraps after slot30; unlimited hides it.
inline std::optional<u8> force_budget_marker_slot(s32 prefix){
    if(prefix<0||prefix>30)return std::nullopt;
    return u8(prefix==30?0:prefix);
}
// Latest footage clarification confirms the source start + end pair. When both
// positions coincide, draw that slot once rather than overpainting it twice.
inline bool force_budget_marker_visible(s32 prefix,unsigned slot){const auto end=force_budget_marker_slot(prefix);return slot<30&&end&&(slot==0||slot==*end);}
//801C2360 midpoint before a slot. The original arithmetic interpolates the
// table's signed angles then stores16 bits; do not substitute shortest-arc math.
inline bool force_budget_marker_pose(const ForceEditorArt& art,unsigned slot,unsigned turn,ForceBudgetMarkerPose* out){
    if(!out||slot>=30||turn>=30)return false;
    const unsigned next=(slot+30-turn)%30,previous=(next+29)%30;
    if(!std::isfinite(art.offsets[previous])||!std::isfinite(art.offsets[next]))return false;
    const auto a=art.angles[previous],b=art.angles[next];const s32 half=s32(f32(f32(s32(b)-s32(a))*.5f));
    const f32 offset=std::fma(.5f,f32(art.offsets[next]-art.offsets[previous]),art.offsets[previous]);if(!std::isfinite(offset))return false;
    *out={offset,std::bit_cast<s16>(u16(s32(a)+half))};return true;
}
//801C2B70 settled-turn branch: visible segments cover turn-3..turn+7; the
// signed comparison draws only slots strictly before the admitted prefix.
inline bool force_budget_band_samples(const ForceBudgetBandArt& art,s32 prefix,unsigned turn,u32 tick,std::vector<ForceBudgetBandSample>* out){
    if(!out)return false;
    out->clear();if(prefix<-1||prefix>30||turn>=30||art.parts.size()!=11||!art.pulse_step||!std::isfinite(art.angle_unit)||art.angle_unit<=0)return false;
    if(prefix<0)return true;
    const auto phase=std::bit_cast<s16>(u16((tick+1)*art.pulse_step));const f32 pulse=std::clamp(std::fma(std::sin(phase*art.angle_unit),.5f,.5f),0.f,1.f);
    for(unsigned i=0;i<art.parts.size();++i){const auto slot=(turn+i+27)%30;if(slot<unsigned(prefix))out->push_back({art.parts[i].frame,art.parts[i].angle,u8(slot),pulse});}
    return true;
}
} // namespace gf
