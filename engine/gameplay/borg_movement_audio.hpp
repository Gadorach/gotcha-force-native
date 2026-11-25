#pragma once
#include "engine/assets/borg_movement_pcm.hpp"
#include "engine/audio/pcm_mixer.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"

namespace gf {
inline constexpr unsigned kBorgMovementAudioFrames=800;
using BorgMovementSoundBank=std::array<std::vector<s16>,4>;
using BorgLaunchSoundBank=std::array<std::vector<s16>,kBorgBattleAttacks>;
using BorgImpactSoundBank=std::array<std::vector<s16>,kBorgBattleAttacks>;
struct BorgReleaseSound {u8 move=255;u16 frame=0;std::vector<s16> pcm;};
using BorgReleaseSoundBank=std::array<BorgReleaseSound,kBorgReleaseSounds>;
using BorgActionSoundBank=std::array<std::vector<s16>,kBorgBattleMoves>;

// Samples accepted cues exactly once at runtime. The common mixer owns
// summing/clipping; this owns only the per-actor cue and looping cursors.
class BorgMovementAudio {
public:
    void reset(){voices_={};shot_voices_={};scope_.reset();tick_=0;launch_started_=release_started_=impact_started_=0;}
    unsigned launch_started()const{return launch_started_;}
    unsigned release_started()const{return release_started_;}
    unsigned impact_started()const{return impact_started_;}
    bool visit(const NativeBattleMvp& world,const std::array<const BorgMovementSoundBank*,2>& banks,std::vector<s16>* out,
               const std::array<const BorgLaunchSoundBank*,2>& launches={},
                const std::array<const BorgReleaseSoundBank*,2>& releases={},
                const std::array<const BorgImpactSoundBank*,2>& impacts={},
                const std::array<const BorgActionSoundBank*,2>& actions={}){
        if(!out||!world.ready()||world.paused()||!world.tick())return false;
        const auto scope=world.presentation_scope();
        if(!world.owns_motion_scope(scope_)){reset();scope_=scope;}
        if(world.tick()<=tick_)return false;
        tick_=world.tick();out->clear();launch_started_=release_started_=impact_started_=0;
        if(!mixer_.begin(kBorgMovementAudioFrames))return false;
        bool mixed=false;
        for(unsigned slot=0;slot<world.actor_count();++slot){
            if(!banks[slot])continue;
            const auto& actor=world.actors()[slot];
            for(unsigned i=0;i<4;++i){
                const auto& clip=(*banks[slot])[i];auto& voice=voices_[slot][i];
                if(clip.empty())continue;
                const bool loop=i==3,cue=(actor.package_movement_cues&(1u<<i))!=0;
                if(!actor.health||(loop&&!cue)){voice={};continue;}
                if(cue&&(!loop||!voice.active))voice={true,0};
                if(!voice.active)continue;
                std::array<s16,kBorgMovementAudioFrames*2> block{};
                for(auto& sample:block){
                    sample=clip[voice.cursor++];
                    if(voice.cursor==clip.size()){
                        voice.cursor=0;
                        if(!loop){voice.active=false;break;}
                    }
                }
                if(!mixer_.add(AudioBus::Sfx,block,.5f))return false;
                mixed=true;
            }
        }
        for(const auto& event:world.package_events()){
            if(event.actor>=shot_voices_.size())continue;
            const bool release=event.kind==BorgBattleEventKind::ReleaseSound;
            const bool impact=event.kind==BorgBattleEventKind::ImpactSound;
            const bool action=event.kind==BorgBattleEventKind::ActionSound;
            unsigned clip=event.value;
            if(release){
                const auto* bank=releases[event.actor];
                if(!bank||event.object)continue;
                const auto found=std::find_if(bank->begin(),bank->end(),[&](const auto& sample){
                    return sample.move==event.move&&sample.frame==event.frame&&!sample.pcm.empty();
                });
                if(found==bank->end())continue;
                clip=unsigned(found-bank->begin());
            }else if(action){
                if(!actions[event.actor]||clip>=kBorgBattleMoves||(*actions[event.actor])[clip].empty())continue;
            }else if(impact){
                if(!event.object||!impacts[event.actor]||clip>=kBorgBattleAttacks||(*impacts[event.actor])[clip].empty())continue;
            }else if(event.kind!=BorgBattleEventKind::LaunchSound||!event.object||!launches[event.actor]||
                       clip>=kBorgBattleAttacks||(*launches[event.actor])[clip].empty())continue;
            auto& voices=shot_voices_[event.actor];
            auto slot=std::find_if(voices.begin(),voices.end(),[](const auto& v){return !v.active;});
            if(slot==voices.end())continue; // saturate without stealing or affecting simulation
            *slot={true,release?ShotCue::Release:impact?ShotCue::Impact:action?ShotCue::Action:ShotCue::Launch,u8(clip),0};
            if(release)++release_started_;
            else if(impact)++impact_started_;
            else if(!action)++launch_started_;
        }
        for(unsigned slot=0;slot<shot_voices_.size();++slot)for(auto& voice:shot_voices_[slot]){
            if(!voice.active)continue;
            const auto* sample=voice.cue==ShotCue::Release?(releases[slot]?&(*releases[slot])[voice.clip].pcm:nullptr):
                               voice.cue==ShotCue::Impact?(impacts[slot]?&(*impacts[slot])[voice.clip]:nullptr):
                               voice.cue==ShotCue::Action?(actions[slot]?&(*actions[slot])[voice.clip]:nullptr):
                               (launches[slot]?&(*launches[slot])[voice.clip]:nullptr);
            if(!sample){voice={};continue;}
            const auto& clip=*sample;
            if(voice.cursor>=clip.size()){voice={};continue;}
            std::array<s16,kBorgMovementAudioFrames*2> block{};
            const auto take=std::min(block.size(),clip.size()-voice.cursor);
            std::copy_n(clip.data()+voice.cursor,take,block.data());
            voice.cursor+=take;
            if(voice.cursor==clip.size())voice={};
            if(!mixer_.add(AudioBus::Sfx,block,.35f))return false;
            mixed=true;
        }
        if(!mixed)return true;
        return mixer_.finish(out);
    }
private:
    struct Voice {bool active=false;std::size_t cursor=0;};
    std::array<std::array<Voice,4>,2> voices_{};
    enum class ShotCue:u8 {Launch,Release,Impact,Action};
    struct ShotVoice {bool active=false;ShotCue cue=ShotCue::Launch;u8 clip=0;std::size_t cursor=0;};
    // At most 8 movement voices + 16 action voices per shared mixer block.
    std::array<std::array<ShotVoice,8>,2> shot_voices_{};
    unsigned launch_started_=0,release_started_=0,impact_started_=0;
    std::weak_ptr<const NativeBorgMotionScope> scope_;
    u32 tick_=0;
    PcmMixer mixer_;
};
}
