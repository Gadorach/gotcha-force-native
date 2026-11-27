#pragma once
#include <span>
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
inline constexpr unsigned kBorgContactSoundClips=kBorgContactSoundBands*2;
using BorgContactSoundBank=std::array<std::vector<s16>,kBorgBattleMoves*kBorgContactSoundClips>;

// One borrowed, immutable set per physical actor for the life of the
// round. Two actors can share sample data without sharing voice cursors.
struct BorgAudioSlot {
    const BorgMovementSoundBank* movement=nullptr;
    const BorgLaunchSoundBank* launch=nullptr;
    const BorgReleaseSoundBank* release=nullptr;
    const BorgImpactSoundBank* impact=nullptr;
    const BorgActionSoundBank* action=nullptr;
    const BorgContactSoundBank* contact=nullptr;
    bool operator==(const BorgAudioSlot&)const=default;
};

// Samples accepted cues exactly once at runtime. The common mixer owns
// summing/clipping; this owns only the per-actor cue and looping cursors.
class BorgMovementAudio {
public:
    void reset(){voices_={};shot_voices_={};bindings_={};scope_.reset();tick_=movement_slots_=0;clear_visit();}
    unsigned launch_started()const{return launch_started_;}
    unsigned release_started()const{return release_started_;}
    unsigned impact_started()const{return impact_started_;}
    unsigned contact_started()const{return contact_started_;}
    bool contact_nonzero()const{return contact_nonzero_;}
    unsigned mixed_actors()const{return mixed_actors_;}
    unsigned dropped()const{return dropped_;}
    bool visit(const NativeBattleMvp& world,std::span<const BorgAudioSlot> slots,std::vector<s16>* out){
        if(!out||!world.ready()||world.paused()||!world.tick()||slots.size()!=world.actor_count())return false;
        static_assert(kBattleMvpMaxActors*4u<=PcmMixer::kMaxSources);
        const bool same_round=world.owns_motion_scope(scope_);
        if(same_round&&!std::equal(slots.begin(),slots.end(),bindings_.begin()))return false;
        unsigned movement_slots=0;
        for(const auto& slot:slots)if(slot.movement)
            for(const auto& clip:*slot.movement)movement_slots+=!clip.empty();
        if(!same_round){
            reset();scope_=world.presentation_scope();
            std::copy(slots.begin(),slots.end(),bindings_.begin());movement_slots_=movement_slots;
        }else if(movement_slots!=movement_slots_)return false;
        if(world.tick()<=tick_)return false;
        tick_=world.tick();out->clear();clear_visit();
        if(!mixer_.begin(kBorgMovementAudioFrames))return false;
        for(unsigned slot=0;slot<slots.size();++slot){
            const auto* bank=slots[slot].movement;
            if(!bank)continue;
            const auto& actor=world.actors()[slot];
            for(unsigned i=0;i<bank->size();++i){
                const auto& clip=(*bank)[i];auto& voice=voices_[slot][i];
                if(clip.empty()||voice.cursor>=clip.size())voice={};
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
                mixed_actors_|=1u<<slot;
            }
        }
        // Reserve space for every bound movement lane, even when silent.
        // Continuing cues never have to be stolen to make a later footstep.
        const unsigned cue_limit=PcmMixer::kMaxSources-movement_slots_;
        unsigned active_cues=0;
        for(const auto& voices:shot_voices_)for(const auto& voice:voices)active_cues+=voice.active;
        for(const auto& event:world.package_events()){
            if(event.actor>=slots.size())continue;
            const auto& banks=slots[event.actor];
            ShotCue kind;
            unsigned clip=event.value;
            switch(event.kind){
            case BorgBattleEventKind::ReleaseSound:{
                const auto* bank=banks.release;
                if(!bank||event.object)continue;
                const auto found=std::find_if(bank->begin(),bank->end(),[&](const auto& sample){
                    return sample.move==event.move&&sample.frame==event.frame&&!sample.pcm.empty();
                });
                if(found==bank->end())continue;
                clip=unsigned(found-bank->begin());kind=ShotCue::Release;break;
            }
            case BorgBattleEventKind::ActionSound:kind=ShotCue::Action;break;
            case BorgBattleEventKind::ContactSound:
                if(!event.object||event.move>=kBorgBattleMoves||clip>=kBorgContactSoundClips)continue;
                clip+=event.move*kBorgContactSoundClips;
                kind=ShotCue::Contact;break;
            case BorgBattleEventKind::ImpactSound:
                if(!event.object)continue;
                kind=ShotCue::Impact;break;
            case BorgBattleEventKind::LaunchSound:
                if(!event.object)continue;
                kind=ShotCue::Launch;break;
            default:continue;
            }
            const auto* sample=cue_sample(banks,kind,clip);
            if(!sample||sample->empty())continue;
            auto& voices=shot_voices_[event.actor];
            auto slot=std::find_if(voices.begin(),voices.end(),[](const auto& v){return !v.active;});
            if(slot==voices.end()||active_cues>=cue_limit){++dropped_;continue;}
            *slot={true,kind,u8(clip),0};
            ++active_cues;
            if(kind==ShotCue::Release)++release_started_;
            else if(kind==ShotCue::Impact)++impact_started_;
            else if(kind==ShotCue::Contact)++contact_started_;
            else if(kind==ShotCue::Launch)++launch_started_;
        }
        for(unsigned slot=0;slot<slots.size();++slot)for(auto& voice:shot_voices_[slot]){
            if(!voice.active)continue;
            const auto* sample=cue_sample(slots[slot],voice.cue,voice.clip);
            if(!sample){voice={};continue;}
            const auto& clip=*sample;
            if(voice.cursor>=clip.size()){voice={};continue;}
            std::array<s16,kBorgMovementAudioFrames*2> block{};
            const auto take=std::min(block.size(),clip.size()-voice.cursor);
            std::copy_n(clip.data()+voice.cursor,take,block.data());
            if(voice.cue==ShotCue::Contact&&!contact_nonzero_)
                contact_nonzero_=std::any_of(block.begin(),block.begin()+take,[](s16 n){return n!=0;});
            voice.cursor+=take;
            if(voice.cursor==clip.size())voice={};
            if(!mixer_.add(AudioBus::Sfx,block,.35f))return false;
            mixed_actors_|=1u<<slot;
        }
        if(!mixed_actors_)return true;
        return mixer_.finish(out);
    }
private:
    struct Voice {bool active=false;std::size_t cursor=0;};
    std::array<std::array<Voice,4>,kBattleMvpMaxActors> voices_{};
    enum class ShotCue:u8 {Launch,Release,Impact,Action,Contact};
    struct ShotVoice {bool active=false;ShotCue cue=ShotCue::Launch;u8 clip=0;std::size_t cursor=0;};
    std::array<std::array<ShotVoice,8>,kBattleMvpMaxActors> shot_voices_{};
    std::array<BorgAudioSlot,kBattleMvpMaxActors> bindings_{};
    static const std::vector<s16>* cue_sample(const BorgAudioSlot& banks,ShotCue kind,unsigned clip){
        const auto lookup=[&](const auto* bank)->const std::vector<s16>*{
            return bank&&clip<bank->size()?&(*bank)[clip]:nullptr;
        };
        switch(kind){
        case ShotCue::Launch:return lookup(banks.launch);
        case ShotCue::Impact:return lookup(banks.impact);
        case ShotCue::Action:return lookup(banks.action);
        case ShotCue::Contact:return lookup(banks.contact);
        case ShotCue::Release:return banks.release&&clip<banks.release->size()?&(*banks.release)[clip].pcm:nullptr;
        }
        return nullptr;
    }
    void clear_visit(){
        launch_started_=release_started_=impact_started_=contact_started_=mixed_actors_=dropped_=0;
        contact_nonzero_=false;
    }
    unsigned launch_started_=0,release_started_=0,impact_started_=0,contact_started_=0;
    unsigned mixed_actors_=0,dropped_=0,movement_slots_=0;
    bool contact_nonzero_=false;
    std::weak_ptr<const NativeBorgMotionScope> scope_;
    u32 tick_=0;
    PcmMixer mixer_;
};
}
