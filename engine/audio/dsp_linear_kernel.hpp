#pragma once
// Shared bounded type1 SRC arithmetic for strict windows and framed voices.
// Codec/end-of-source policy belongs to their caller, not this numeric kernel.
#include <array>
#include <limits>
#include <span>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct DspLinearState {
    u32 ratio=0,fraction=0;std::array<s16,4> history{};u64 output_samples=0;
    bool operator==(const DspLinearState&)const=default;
};
namespace dsp_linear_detail {
inline u32 source_count(const DspLinearState& state,unsigned requested){return u32((u64(state.fraction)+u64(requested)*state.ratio)>>16);}
inline bool render(const DspLinearState& state,std::span<const s16> input,unsigned requested,DspLinearState* next,std::vector<s16>* out){
    if(!next||!out||!requested||requested>4096||state.ratio>0x40000u||state.fraction>65535u||state.output_samples>std::numeric_limits<u64>::max()-requested||input.size()!=source_count(state,requested))return false;
    auto candidate=state;std::vector<s16> result;result.reserve(requested);unsigned used=0;
    for(unsigned i=0;i<requested;++i){const u32 phase=candidate.fraction+candidate.ratio;candidate.fraction=phase&65535u;
        for(unsigned j=0;j<(phase>>16);++j){for(unsigned k=0;k<3;++k)candidate.history[k]=candidate.history[k+1];candidate.history[3]=input[used++];}
        const s64 sum=s64(candidate.history[0])*s64(65536u-candidate.fraction)+s64(candidate.history[1])*candidate.fraction;
        result.push_back(s16(sum>=0?sum/65536:-((-sum+65535)/65536)));
    }
    if(used!=input.size())return false;
    candidate.output_samples+=requested;*next=candidate;*out=std::move(result);return true;
}
} // namespace dsp_linear_detail
} // namespace gf
