#pragma once
#include "engine/assets/challenge_stage_rng.hpp"
#include "engine/gameplay/source_byte_rng.hpp"
namespace gf {
struct ChallengeStageRngPlan {
    u8 stage=0,field_1d=0,field_1e=0;u32 draws=0;SourceByteRngState next;
    bool operator==(const ChallengeStageRngPlan&)const=default;
};
//80196DAC: AFTER selected-human rosters, BEFORE later Challenge AI assembly.
//Pure candidate only: no global RNG, entry reset, scene clock, resource loading,
//party readiness, or final-state commit. Rejected-stage draws still advance the
//candidate stream. A stuck stream REFUSES instead of choosing a substitute.
inline bool challenge_stage_rng_plan(const ChallengeStageRngProfile& profile,SourceByteRngState input,s8 previous,ChallengeStageRngPlan* out){
    if(out)*out={};
    if(!out||!profile.valid())return false;
    ChallengeStageRngPlan plan;plan.next=input;
    //Native safety budget, not a recovered source retry/lifetime limit. Failure
    //publishes no partial plan and consumes no caller-owned RNG state.
    for(unsigned retry=0;retry<8192;++retry){
        const unsigned row=source_byte_rng_step(plan.next)%profile.rows.size();++plan.draws;
        const auto& selected=profile.rows[row];
        if(int(selected.stage)==int(previous))continue;
        plan.stage=selected.stage;plan.field_1d=u8(source_byte_rng_step(plan.next)%3u);++plan.draws;
        plan.field_1e=u8(source_byte_rng_step(plan.next)%selected.field_1e_count);++plan.draws;
        *out=plan;return true;
    }
    return false;
}
} // namespace gf
