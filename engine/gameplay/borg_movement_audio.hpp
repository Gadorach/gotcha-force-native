#pragma once
#include "engine/audio/pcm_mixer.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"

namespace gf {
inline constexpr unsigned kBorgMovementSampleRate=48000,kBorgMovementAudioFrames=800;
inline constexpr std::size_t kBorgMovementSoundBytes=kBorgMovementSampleRate*4*3;
using BorgMovementSoundBank=std::array<std::vector<s16>,4>;

// Authored PCM only: stereo 48 kHz signed 16-bit little endian, up to 3 s.
inline bool borg_movement_sound_decode(std::string_view bytes,std::vector<s16>* out){
    if(!out||bytes.empty()||bytes.size()>kBorgMovementSoundBytes||bytes.size()%4)return false;
    std::vector<s16> result(bytes.size()/2);
    for(std::size_t i=0;i<result.size();++i)
        result[i]=std::bit_cast<s16>(u16(u8(bytes[2*i])|(u16(u8(bytes[2*i+1]))<<8)));
    *out=std::move(result);return true;
}
// Samples accepted cues exactly once at runtime. The common mixer owns
// summing/clipping; this owns only the per-actor cue and looping cursors.
class BorgMovementAudio {
public:
    void reset(){voices_={};scope_.reset();tick_=0;}
    bool visit(const NativeBattleMvp& world,const std::array<const BorgMovementSoundBank*,2>& banks,std::vector<s16>* out){
        if(!out||!world.ready()||world.paused()||!world.tick())return false;
        const auto scope=world.presentation_scope();
        if(!world.owns_motion_scope(scope_)){reset();scope_=scope;}
        if(world.tick()<=tick_)return false;
        tick_=world.tick();out->clear();
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
        if(!mixed)return true;
        return mixer_.finish(out);
    }
private:
    struct Voice {bool active=false;std::size_t cursor=0;};
    std::array<std::array<Voice,4>,2> voices_{};
    std::weak_ptr<const NativeBorgMotionScope> scope_;
    u32 tick_=0;
    PcmMixer mixer_;
};
}
