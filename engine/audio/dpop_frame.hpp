#pragma once
// Source218980 accumulation +21855C signed rolloff and AX setup-buffer model.
// One160-sample/32kHz audio-domain frame, not simulation timing or queue ACK.
#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct DpopState {std::array<s32,9> accumulator{};u64 frames=0;bool operator==(const DpopState&)const=default;};
struct DpopFrame {std::array<s32,9> initial{};std::array<s16,9> delta{};std::array<u8,54> source_bytes{};};
struct DpopPlan {DpopState state;DpopFrame frame;};
namespace dpop_detail {
inline s32 add(s32 a,s32 b){return std::bit_cast<s32>(std::bit_cast<u32>(a)+std::bit_cast<u32>(b));}
inline s32 sample(const DpopFrame& f,unsigned lane,unsigned position){return f.initial[lane]?std::bit_cast<s32>(u32(s64(f.initial[lane])+s64(position)*f.delta[lane])):0;}
}
inline bool dpop_prepare(const DpopState& input,std::span<const std::array<s16,9>> contributions,DpopPlan* out,std::string* error){
    if(out && &input==&out->state){if(error)*error="dpop: aliased state/output";return false;}
    if(out&&contributions.size()<=64&&contributions.data())for(const auto& row:contributions)if(&row==&out->frame.delta){if(error)*error="dpop: aliased contribution/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("dpop: ")+why;return false;};
    if(!out||contributions.size()>64||(!contributions.empty()&&!contributions.data())||input.frames==std::numeric_limits<u64>::max())return fail("frame/output/physical-slot budget");
    DpopPlan plan;plan.state=input;
    for(const auto& row:contributions)for(unsigned lane=0;lane<9;++lane)plan.state.accumulator[lane]=dpop_detail::add(plan.state.accumulator[lane],row[lane]);
    for(unsigned lane=0;lane<9;++lane){const s32 value=plan.state.accumulator[lane];const s32 quotient=value/160;
        if(!quotient){plan.state.accumulator[lane]=0;continue;} // source discards sub160 residual before this frame
        const s32 step=std::clamp(quotient,-20,20);plan.frame.initial[lane]=value;plan.frame.delta[lane]=s16(-step);plan.state.accumulator[lane]=s32(s64(value)-s64(step)*160);
    }
    // Wire order is mainL/R/S, auxA L/R/S, auxB L/R/S, not logical mix order.
    constexpr std::array<unsigned,9> order={0,1,7,2,3,8,4,5,6};
    for(unsigned wire=0;wire<9;++wire){const auto lane=order[wire];store_be32(plan.frame.source_bytes.data()+wire*6,std::bit_cast<u32>(plan.frame.initial[lane]));store_be16(plan.frame.source_bytes.data()+wire*6+4,std::bit_cast<u16>(plan.frame.delta[lane]));}
    ++plan.state.frames;*out=plan;if(error)error->clear();return true;
}
} // namespace gf
