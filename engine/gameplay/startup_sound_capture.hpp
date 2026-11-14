#pragma once
// Immutable per-accepted-frame source call INTENTS, not allocation/playback ACK.
// Actor keys1/2 are local to this weak native capture scope, never guest pointers.
#include <memory>
#include <vector>
#include "engine/gameplay/startup_sound_spatial.hpp"
#include "engine/gameplay/ui_sound_operations.hpp"
namespace gf {
struct CharacterModelBinding;
struct StartupSoundCaptureScope {std::array<std::weak_ptr<const CharacterModelBinding>,2> actors;bool manager_producers=false;};
enum class StartupSoundIntentOrigin:u8 {Director,PropPre,RevealPre,ActorExit,ActorScript,PropPost,RevealPost,ActorManagerPre,ManagerTail};
enum class StartupSoundIntentKind:u8 {Sfx,MusicCommand};
struct StartupSoundIntent {
    StartupSoundIntentOrigin origin=StartupSoundIntentOrigin::Director;StartupSoundIntentKind kind=StartupSoundIntentKind::Sfx;
    u8 actor=255,part=255,lane=255,script=255,opcode=0;u16 operand=0;u32 command_offset=0;
    StartupSoundPendingCall call;bool bypass_disabled=false;
};
struct StartupSoundManagerCapture {
    u8 actor=255;bool pre_actor=false,tick_sampled=false;
    StartupSoundOperation operation;StartupSoundTickSample clock;
    u32 parent_frame_before=0;u8 native_parent708=0,construction708=0,pre709=0,pre707=0;
    bool requires_parent_sound_completion=true;
};
struct StartupSoundCaptureFrame {
    std::weak_ptr<const StartupSoundCaptureScope> scope;u32 frame=0;
    std::array<std::weak_ptr<const CharacterModelBinding>,2> actors;
    std::array<StartupSoundOwnerObservation,2> owners;
    std::vector<StartupSoundIntent> intents;UiSoundBatch ui;
    std::vector<StartupSoundManagerCapture> manager_actions;
    StartupSoundState manager_candidate;bool manager_producers=false;
    bool requires_live_spatial_settings_audio_consumer=true;
};
inline bool startup_sound_intent_append(StartupSoundCaptureFrame* frame,StartupSoundIntent intent){
    if(!frame||frame->intents.size()>=48)return false;
    frame->intents.push_back(std::move(intent));return true;
}
} // namespace gf
