#pragma once
// Explicit capture-scope/frame-bound lowerer. SFX -> existing spatial/command
// core; director music remains ordered pending metadata, never SFX/PCM ACK.
#include "engine/gameplay/startup_sound_capture.hpp"
#include "engine/gameplay/ui_sound_commands.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/gameplay/startup_sound_no_viewers.hpp"
namespace gf {
struct StartupSoundCaptureContext {
    std::weak_ptr<const StartupSoundCaptureScope> scope;u32 frame=0;
    StartupSoundCommandObservation commands;StartupSoundSpatialSnapshot spatial;
    StartupSoundNoViewerFrame no_viewers;
};
struct StartupPendingMusic {u16 order=0;u8 opcode=0;u16 operand=0;u32 command_offset=0;};
struct StartupSoundCapturePlan {
    StartupSoundCommandQueue candidate;std::vector<StartupSoundScalarCommand> scalar_calls;
    std::vector<StartupPendingMusic> music;unsigned ui_operations=0;
    bool requires_audio_and_music_consumer=true;
};
namespace startup_sound_capture_commands_detail {
inline bool plan(const SoundCommandBanks& banks,const StartupSoundSpatialProfile& spatial_profile,
        const StartupSoundCaptureFrame& frame,const StartupSoundCaptureContext& context,const StartupSoundCommandQueue& input,
        bool manager_mode,StartupSoundCapturePlan* out,std::string* error){
    if(out && &input==&out->candidate){if(error)*error="startup sound capture commands: aliased queue/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound capture commands: ")+why;return false;};
    const auto capture=frame.scope.lock(),observed=context.scope.lock();
    if(!out||!capture||capture!=observed||!frame.frame||frame.frame!=context.frame||frame.intents.size()>48)return fail("live capture scope/frame/intent budget");
    if(manager_mode){if(!capture->manager_producers||!frame.manager_producers||!context.no_viewers.live()||context.no_viewers.epoch()!=frame.frame)return fail("explicit managed no-viewer producer frame required");}
    else if(capture->manager_producers||frame.manager_producers)return fail("managed source frame requires explicit managed lowerer");
    StartupSoundCapturePlan plan;plan.candidate=input;
    for(unsigned order=0;order<frame.intents.size();++order){const auto& intent=frame.intents[order];
        if(intent.kind==StartupSoundIntentKind::MusicCommand){if(intent.origin!=StartupSoundIntentOrigin::Director||(intent.opcode!=19&&intent.opcode!=20)||intent.bypass_disabled||intent.call.owner)return fail("source pending music command");plan.music.push_back({u16(order),intent.opcode,intent.operand,intent.command_offset});continue;}
        if(intent.kind!=StartupSoundIntentKind::Sfx)return fail("unknown intent kind");
        switch(intent.origin){
        case StartupSoundIntentOrigin::Director:if(intent.opcode!=21||intent.call.cue!=s16(intent.operand)||intent.call.owner||intent.bypass_disabled)return fail("source director SFX operands");break;
        case StartupSoundIntentOrigin::PropPre:case StartupSoundIntentOrigin::PropPost:if(intent.part>=8||(intent.call.cue!=378&&intent.call.cue!=379)||intent.call.owner||!intent.bypass_disabled)return fail("source prop/null ungated sound");break;
        case StartupSoundIntentOrigin::RevealPre:case StartupSoundIntentOrigin::RevealPost:if(intent.part>=3||intent.call.cue!=377||intent.call.owner||!intent.bypass_disabled)return fail("source late-reveal/null ungated sound");break;
        case StartupSoundIntentOrigin::ActorExit:if(intent.actor>=2||intent.call.owner!=u64(intent.actor)+1||intent.call.cue!=242||intent.bypass_disabled)return fail("source actor exit call");break;
        case StartupSoundIntentOrigin::ActorScript:if(intent.actor>=2||intent.lane>=2||intent.call.owner!=u64(intent.actor)+1||intent.bypass_disabled||(intent.script!=26&&intent.script!=27&&intent.script!=29&&intent.script!=30))return fail("source actor script identity");break;
        case StartupSoundIntentOrigin::ActorManagerPre:if(!manager_mode||intent.actor!=0||intent.call.owner!=1||intent.bypass_disabled||intent.call.kind!=StartupSoundCallKind::StartBiased||intent.call.cue!=40)return fail("native pre-VM descriptor0 manager start");break;
        case StartupSoundIntentOrigin::ManagerTail:if(!manager_mode||intent.actor>=2||intent.call.owner!=u64(intent.actor)+1||intent.bypass_disabled||!((intent.call.kind==StartupSoundCallKind::UpdateBiased&&intent.call.cue==40)||(intent.call.kind==StartupSoundCallKind::StartDirect&&intent.call.cue==41)))return fail("native descriptor0 manager tail update/stop");break;
        default:return fail("unsupported source origin");
        }
        const bool manager_call=intent.origin==StartupSoundIntentOrigin::ActorManagerPre||intent.origin==StartupSoundIntentOrigin::ManagerTail;
        if(!manager_call&&intent.call.kind!=StartupSoundCallKind::StartDirect&&(intent.origin!=StartupSoundIntentOrigin::ActorScript||intent.call.kind!=StartupSoundCallKind::StartBiased))return fail("source start-only call kind");
        StartupSoundScalarCommand scalar;
        if(intent.call.owner){const unsigned actor=intent.actor;const auto binding=frame.actors[actor].lock();if(!binding||binding!=capture->actors[actor].lock()||frame.owners[actor].generation!=intent.call.owner||frame.owners[actor].epoch!=frame.frame)return fail("actor/resource/snapshot lifetime");
            if(manager_mode){if(!startup_sound_no_viewers_resolve(spatial_profile,context.no_viewers,intent.call,frame.owners,&scalar,error))return false;}
            else{if(context.spatial.epoch!=frame.frame)return fail("actor/resource/snapshot lifetime");if(!startup_sound_spatial_resolve(spatial_profile,intent.call,context.spatial,frame.owners,&scalar,error))return false;}
        }
        else{if(intent.call.kind!=StartupSoundCallKind::StartDirect||intent.call.viewer_bias!=0||intent.call.cue<0||intent.call.cue>=384)return fail("qualified null direct start");scalar.cue=intent.call.cue;scalar.volume=127;scalar.pan=64;}
        auto observation=context.commands;if(intent.bypass_disabled){observation.gate_declared=true;observation.disabled=0;}
        StartupSoundCommandPlan lowered;if(!sound_command_plan(banks,plan.candidate,observation,std::span(&scalar,1),&lowered,error))return false;plan.candidate=std::move(lowered.candidate);plan.scalar_calls.push_back(scalar);
    }
    UiSoundCommandPlan ui;if(!ui_sound_command_plan(banks,plan.candidate,context.commands,frame.ui,&ui,error))return false;plan.candidate=std::move(ui.candidate);plan.ui_operations=ui.count;*out=std::move(plan);if(error)error->clear();return true;
}
} // namespace startup_sound_capture_commands_detail
inline bool startup_sound_capture_commands(const SoundCommandBanks& banks,const StartupSoundSpatialProfile& spatial_profile,
        const StartupSoundCaptureFrame& frame,const StartupSoundCaptureContext& context,const StartupSoundCommandQueue& input,
        StartupSoundCapturePlan* out,std::string* error){
    return startup_sound_capture_commands_detail::plan(banks,spatial_profile,frame,context,input,false,out,error);
}
inline bool startup_sound_capture_managed_commands(const SoundCommandBanks& banks,const StartupSoundSpatialProfile& spatial_profile,
        const StartupSoundCaptureFrame& frame,const StartupSoundCaptureContext& context,const StartupSoundCommandQueue& input,
        StartupSoundCapturePlan* out,std::string* error){
    return startup_sound_capture_commands_detail::plan(banks,spatial_profile,frame,context,input,true,out,error);
}
} // namespace gf
