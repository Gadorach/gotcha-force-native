#pragma once
// 801C83E4 -> E99C8 immediate play request; 801C8434 -> E9B08 -> E9138
// pause lane1. Ordered candidates, NOT original async queue or PCM/speaker ACK.
#include "engine/assets/startup_music.hpp"
#include "engine/gameplay/startup_sound_capture.hpp"
namespace gf {
enum class StartupMusicCommandKind:u8 {Start,Pause};
struct StartupMusicCommand {
    StartupMusicCommandKind kind=StartupMusicCommandKind::Start;
    u32 frame=0,intent_order=0,command_offset=0,member=0;
    s16 script_index=-1;u8 archive=0,player=2,pause_lane=1,pause_value=0;
};
struct StartupMusicCommandState {
    u32 frame=0;u64 commands=0,starts=0;s32 member=-1;
    std::array<u8,2> pause_lanes{};bool transport_paused=false;
    StartupMusicProfile bindings;std::weak_ptr<const StartupSoundCaptureScope> scope;
};
struct StartupMusicCommandPlan {
    StartupMusicCommandState candidate;std::vector<StartupMusicCommand> commands;
    bool requires_native_pcm_consumer=true;
};
inline bool startup_music_command_plan(const StartupMusicProfile& bindings,const StartupMusicCommandState& state,
        const StartupSoundCaptureFrame& capture,StartupMusicCommandPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup music commands: ")+why;return false;};
    const auto scope=capture.scope.lock();
    if(!out||!bindings.ready()||!scope||!capture.frame||capture.frame>8192||capture.frame!=state.frame+1||capture.intents.size()>48)
        return fail("live binding/capture/consecutive bounded frame required");
    if(state.commands>u64(state.frame)*48||state.starts>state.commands||state.pause_lanes[0]>1||state.pause_lanes[1]>1||
       (state.frame?(!state.bindings.same_bindings(bindings)||state.scope.lock()!=scope):
        (state.commands||state.starts||state.member!=-1||state.pause_lanes[0]||state.pause_lanes[1]||state.transport_paused)))
        return fail("fresh initialized or matching live source candidate required");
    if((state.starts==0)!=(state.member==-1)||(state.starts&&(state.member<0||!bindings.contains_member(u32(state.member)))))
        return fail("selected member/start generation mismatch");
    StartupMusicCommandPlan plan;plan.candidate=state;plan.candidate.bindings=bindings;plan.candidate.scope=scope;
    for(unsigned i=0;i<capture.intents.size();++i){
        const auto& intent=capture.intents[i];
        if(intent.kind==StartupSoundIntentKind::Sfx)continue;
        if(intent.kind!=StartupSoundIntentKind::MusicCommand||intent.origin!=StartupSoundIntentOrigin::Director||intent.actor!=255)
            return fail("only qualified director music intents supported");
        StartupMusicCommand command;command.frame=capture.frame;command.intent_order=i;command.command_offset=intent.command_offset;
        if(intent.opcode==19){
            if(intent.operand>32767)return fail("negative signed source table index unsupported");
            const auto* stream=bindings.resolve(s16(intent.operand));if(!stream)return fail("unbound sparse source music index");
            command.kind=StartupMusicCommandKind::Start;command.script_index=s16(intent.operand);command.member=stream->entry;
            plan.candidate.member=s32(stream->entry);++plan.candidate.starts;
            // Source E8FE0 unpauses the transport before starting the new stream;
            // stored pause lanes are separate and are NOT blanket-reset here.
            plan.candidate.transport_paused=false;
        }else if(intent.opcode==20){
            if(intent.operand>1)return fail("only source pause byte0/1 qualified");
            command.kind=StartupMusicCommandKind::Pause;command.pause_value=u8(intent.operand);
            plan.candidate.pause_lanes[1]=command.pause_value;
            const u8 combined=u8(plan.candidate.pause_lanes[0]+plan.candidate.pause_lanes[1]);
            // E9138 forwards nonzero requests; zero reaches the transport only
            // when BOTH pause lanes sum to zero. It is not a toggle/restart.
            if(command.pause_value||!combined)plan.candidate.transport_paused=command.pause_value!=0;
        }else return fail("unsupported director music opcode");
        ++plan.candidate.commands;plan.commands.push_back(command);
    }
    plan.candidate.frame=capture.frame;*out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
