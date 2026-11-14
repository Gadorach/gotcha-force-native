#pragma once
// 5B880/5B98C sound-only rule. Caller supplies accepted current+previous clocks,
// source end_frame and actorB4; this does NOT fulfill the actor manager/owner.
#include <cmath>
#include "engine/assets/actor_sound_events.hpp"
#include "engine/gameplay/startup_sound_manager.hpp"
namespace gf {
struct ActorSoundClock {f32 elapsed=0,previous_elapsed=0,frame=0,previous_frame=0; s16 end_frame=0;f32 scale_b4=1;};
inline bool actor_sound_events(const ActorSoundEventBank& bank,u8 script_id,const ActorSoundClock& clock,u64 owner,std::vector<StartupSoundPendingCall>* out,std::string* error){
    if(out)out->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("actor sound rule: ")+why;return false;};
    if(!out||!bank.ready()||!owner||clock.end_frame<0||clock.end_frame>8192)return fail("model-bound script/live owner/end domain");
    for(auto v:{clock.elapsed,clock.previous_elapsed,clock.frame,clock.previous_frame})if(!std::isfinite(v)||v<0||v>8192)return fail("accepted clock domain");
    if(!std::isfinite(clock.scale_b4)||clock.scale_b4<=0||clock.scale_b4>16)return fail("source actorB4 domain");
    const auto* script=bank.find(script_id);if(!script)return fail("script not in explicit audited sparse bank");std::vector<StartupSoundPendingCall> calls;
    for(const auto& row:script->rows){const f32 now=row[2]?clock.frame<clock.previous_frame?f32(clock.frame+f32(clock.end_frame)):clock.frame:clock.elapsed;const f32 previous=row[2]?clock.previous_frame:clock.previous_elapsed;const f32 time=f32(load_be16(row.data()));
        const bool hit=time==0?now<=0:now>=time&&previous<time;if(!hit||row[3]==4)continue;
        s32 cue=load_be16(row.data()+4);if(row[3]==1){if(clock.scale_b4<bits_to_f32(0x3f333333u))--cue;else if(clock.scale_b4>2.f)++cue;}
        if(cue<0||cue>=384)return fail("derived cue outside qualified source command domain");
        const bool biased=row[3]==2||row[3]==3;calls.push_back({biased?StartupSoundCallKind::StartBiased:StartupSoundCallKind::StartDirect,s16(cue),owner,biased?bits_to_f32(0x3e4ccccdu):0});
    }
    *out=std::move(calls);if(error)error->clear();return true;
}
} // namespace gf
