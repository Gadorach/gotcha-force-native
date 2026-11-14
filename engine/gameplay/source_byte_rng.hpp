#pragma once
#include "engine/core/types.hpp"
namespace gf {
struct SourceByteRngState {u8 low=0,high=0;bool operator==(const SourceByteRngState&)const=default;};
//800055FC: explicit two-byte state, no global/clock/seed inference. The source
//returns the NEW low byte; high storage truncates after (u16-state*3)>>8.
inline u8 source_byte_rng_step(SourceByteRngState& state){
    const u32 upper=((u32(state.high)*256u+state.low)*3u)>>8;
    state.high=u8(upper);state.low=u8(u32(state.low)+(upper&255u));return state.low;
}
} // namespace gf
