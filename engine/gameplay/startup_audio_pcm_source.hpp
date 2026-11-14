#pragma once
// Explicit native startup music+SFX join. Original async music-queue latency,
// source effect/actor admission and physical playback remain separate gates.
#include "engine/audio/pcm_player.hpp"
#include "engine/gameplay/sound_pcm_source.hpp"
#include "engine/gameplay/startup_music_commands.hpp"
#include "engine/gameplay/startup_sound_delivery.hpp"
namespace gf {
namespace startup_audio_pcm_detail {struct Epoch {u64 commits=0;};}
class StartupAudioPcmSource;
class StartupAudioPcmReceipt {
public:
    bool valid()const{
        const auto epoch=epoch_.lock();return epoch&&sequence_&&sequence_<=epoch->commits&&
            sfx_.valid()&&sfx_.sequence()==sequence_&&view_.valid(nullptr);
    }
    bool matches(const StartupSoundInputView& view)const{return valid()&&view.valid(nullptr)&&sfx_.matches(view.binding())&&sfx_.boundary()<=view.frames();}
    const SoundPcmBoundaryReceipt& sfx()const{return sfx_;}
    u64 applied_music_commands()const{return music_commands_;}
    u32 captured_frame()const{return captured_frame_;}
private:
    friend class StartupAudioPcmSource;
    std::weak_ptr<const startup_audio_pcm_detail::Epoch> epoch_;
    SoundPcmBoundaryReceipt sfx_;StartupSoundInputView view_;
    u64 sequence_=0,music_commands_=0;u32 captured_frame_=0;
};
class StartupAudioInputCompletion {
public:
    bool observe(const StartupSoundInputView& view,const StartupAudioPcmReceipt& receipt,std::string* error){
        if(!receipt.matches(view)||receipt.applied_music_commands()<music_commands_){if(error)*error="startup audio completion: live matching joined receipt required";return false;}
        auto candidate=sfx_;if(!candidate.observe(view,receipt.sfx(),error))return false;
        if(candidate.state().pending_music_commands!=receipt.applied_music_commands()||candidate.state().captured_frame!=receipt.captured_frame()){
            if(error)*error="startup audio completion: covered SFX/music input prefix mismatch";
            return false;
        }
        sfx_=std::move(candidate);music_commands_=receipt.applied_music_commands();if(error)error->clear();return true;
    }
    const StartupSoundSourceDeliveryState& source_state()const{return sfx_.state();}
    u64 applied_music_commands()const{return music_commands_;}
    u64 pending_music_commands()const{return sfx_.state().pending_music_commands-music_commands_;}
private:
    StartupSoundSourceCompletion sfx_;u64 music_commands_=0;
};
class StartupMusicLibrary {
public:
    bool load(const StartupMusicProfile& profile,IFileSystem& files,const std::string& archive,std::string* error){
        if(data_||!profile.ready()){if(error)*error="startup music library: fresh owner/live bindings required";return false;}
        auto next=std::make_shared<Data>();next->profile=profile;
        for(unsigned i=0;i<2;++i){const auto* stream=profile.stream(i);if(!stream||!menu_music_load(files,archive,*stream,&next->audio[i],error))return false;}
        data_=std::move(next);if(error)error->clear();return true;
    }
    bool ready()const{return bool(data_);}
    u32 output_rate()const{return data_?data_->profile.stream(0)->output_rate:0;}
    const StartupMusicProfile& bindings()const{return data_?data_->profile:empty_;}
    const AdxAudio* audio(u32 member)const{
        if(data_)for(unsigned i=0;i<2;++i)if(data_->profile.stream(i)->entry==member)return &data_->audio[i];
        return nullptr;
    }
private:
    struct Data {StartupMusicProfile profile;std::array<AdxAudio,2> audio;};
    std::shared_ptr<const Data> data_;StartupMusicProfile empty_;
};
struct StartupAudioNativePolicy {
    // Host review controls, not source pause-lane writes or original game policy.
    bool hold_music=false,mute_music=false;
};
struct StartupAudioPcmSnapshot {
    SoundPcmSourceSnapshot sfx;StartupMusicCommandState music;PcmPlayerCursor music_cursor;
    u32 music_boundary=0;u64 queued_music_bus_frames=0,generated_music_frames=0;
    bool failed=false,pending=false;
    bool requires_original_effect_and_full_actor_admission=true;
};
struct StartupAudioSfxVoiceDiagnostic {
    u8 backend_slot=0,bank=0;u16 software_slot=0,event=0,program=0,sample=0,dsp=0;
    u16 event_phase=0;u8 envelope_phase=0;u32 envelope_level=0;s32 current_flags=0,requested_flags=0;
    bool running=false,loop=false,loop_info_valid=false;
};
class StartupAudioPcmSource {
public:
    StartupAudioPcmSource()=default;
    StartupAudioPcmSource(const StartupAudioPcmSource&)=delete;
    StartupAudioPcmSource& operator=(const StartupAudioPcmSource&)=delete;
    StartupAudioPcmSource(StartupAudioPcmSource&&)=delete;
    StartupAudioPcmSource& operator=(StartupAudioPcmSource&&)=delete;
    bool load(SoundVoiceLibrary voices,SoundVoiceProfile voice,SoundEnvelopeProfile envelope,StartupMusicLibrary music,std::string* error){
        if(loaded_||failed_||!music.ready()){if(error)*error="startup audio PCM: fresh owner and decoded music library required";return false;}
        if(!sfx_.load(std::move(voices),voice,envelope,music.output_rate(),error))return false;
        music_=std::move(music);epoch_=std::make_shared<startup_audio_pcm_detail::Epoch>();loaded_=true;if(error)error->clear();return true;
    }
    std::shared_ptr<SoundPcmBoundaryBinding> create_boundary_binding(std::string* error){return sfx_.create_boundary_binding(error);}
    bool pump(PcmQueue& queue,PcmMixer& mixer,const StartupSoundInputView& view,std::string* error,StartupAudioNativePolicy policy={}){
        if(!loaded_||failed_||pending_||sfx_.failed())return false;
        return queue.pump(mixer,[&](unsigned frames,PcmMixer& mix){
            std::vector<SoundPcmBoundaryCut> cuts;
            if(!sfx_.prepare_tracked_with_cuts(frames,mix,view,&cuts,error))return false;
            return prepare_music(frames,mix,view,cuts,policy,error);
        },[&](bool accepted){return complete(accepted);});
    }
    SoundPcmBoundaryReceipt boundary_receipt()const{return sfx_.boundary_receipt();}
    StartupAudioPcmReceipt delivery_receipt()const{
        StartupAudioPcmReceipt receipt;if(!epoch_||!epoch_->commits)return receipt;
        receipt.epoch_=epoch_;receipt.sfx_=sfx_.boundary_receipt();receipt.view_=committed_view_;
        receipt.sequence_=epoch_->commits;receipt.music_commands_=state_.commands.commands;receipt.captured_frame_=state_.commands.frame;return receipt;
    }
    StartupAudioPcmSnapshot snapshot()const{return {sfx_.snapshot(),state_.commands,state_.cursor,state_.boundary,state_.bus_frames,state_.generated,failed_||sfx_.failed(),bool(pending_),true};}
    std::vector<StartupAudioSfxVoiceDiagnostic> active_sfx_voices()const{
        std::vector<StartupAudioSfxVoiceDiagnostic> result;const auto backend=sfx_.state_.cpu.backend.snapshot();const auto& life=sfx_.state_.cpu.parameters.lifecycle;
        for(unsigned i=0;i<64;++i)if(backend.allocated[i]){
            const auto& owner=backend.owners[i];const auto& voice=life.voices.voices[owner.slot];const auto& event=life.events.events[owner.event];const auto& seed=voice.seed;
            StartupAudioSfxVoiceDiagnostic d;d.backend_slot=u8(i);d.bank=seed.resource?seed.resource->id:255;d.software_slot=owner.slot;d.event=owner.event;d.program=seed.program;d.sample=seed.sample;d.dsp=seed.dsp;d.event_phase=event.phase0c;d.envelope_phase=life.envelopes[owner.slot].phase;d.envelope_level=life.envelopes[owner.slot].level;d.current_flags=event.current04;d.requested_flags=event.requested08;d.running=backend.running[i];
            if(seed.resource&&seed.dsp<seed.resource->bank.dsp.size()){const auto& header=seed.resource->bank.dsp[seed.dsp].header;DspAdpcmInfo info;d.loop_info_valid=dsp_voice_header({header.data(),header.size()},&info,nullptr);d.loop=info.loop;}
            result.push_back(d);
        }
        return result;
    }
private:
    struct MusicState {StartupMusicCommandState commands;PcmPlayerCursor cursor;u32 boundary=0;u64 bus_frames=0,generated=0;};
    bool prepare_music(unsigned frames,PcmMixer& mixer,const StartupSoundInputView& view,const std::vector<SoundPcmBoundaryCut>& cuts,StartupAudioNativePolicy policy,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("startup audio music: ")+why;return false;};
        if(!view.valid(error)||!frames||frames>4096||pending_||state_.bus_frames>std::numeric_limits<u64>::max()-frames)return fail("live immutable input/one bounded staged block required");
        auto next=state_;std::vector<s16> pcm(std::size_t(frames)*2,0);unsigned offset=0;
        auto render=[&](unsigned end){
            if(end<offset||end>frames)return false;
            const unsigned count=end-offset;
            if(count&&next.commands.member>=0&&next.cursor.playing&&!next.commands.transport_paused&&!policy.hold_music){
                const auto* audio=music_.audio(u32(next.commands.member));std::vector<s16> part;u32 produced=0;
                if(!audio||!pcm_player_render_cursor(*audio,music_.output_rate(),&next.cursor,count,&part,&produced)||next.generated>std::numeric_limits<u64>::max()-produced)return false;
                next.generated+=produced;
                if(!policy.mute_music)std::copy(part.begin(),part.end(),pcm.begin()+std::size_t(offset)*2);
            }
            offset=end;return true;
        };
        for(const auto& cut:cuts){
            if(cut.boundary!=next.boundary+1||!render(cut.output_offset))return fail("nonconsecutive boundary or device-production cut");
            const auto entry=view.find(cut.boundary);if(!entry)return fail("missing immutable source boundary");
            if(entry->captured){
                StartupMusicCommandPlan plan;if(!startup_music_command_plan(music_.bindings(),next.commands,entry->captured->capture,&plan,error))return false;
                for(const auto& command:plan.commands)if(command.kind==StartupMusicCommandKind::Start){
                    if(!music_.audio(command.member))return fail("decoded command member binding missing");
                    next.cursor={};next.cursor.playing=true;
                }
                next.commands=std::move(plan.candidate);
            }
            next.boundary=cut.boundary;
        }
        if(next.boundary!=sfx_.pending_->boundary||!render(frames)||!mixer.add(AudioBus::Bgm,pcm))return fail("joint source prefix/player/shared mixer refusal");
        next.bus_frames+=frames;pending_=std::move(next);pending_view_=view;return true;
    }
    bool complete(bool accepted){
        // Validate both candidates after actual sink acceptance and BEFORE either
        // commit. Serialized owner: no callbacks or owner mutations between these
        // checks and publication. Accepted bytes on refusal remain unreplayed.
        if(!accepted||!pending_||!pending_view_.valid(nullptr)||!sfx_.pending_commit_valid()||!epoch_||
           epoch_->commits==std::numeric_limits<u64>::max()||epoch_->commits!=sfx_.state_.tracked_sequence){
            (void)sfx_.complete_using(false,true);pending_.reset();pending_view_={};failed_=true;return false;
        }
        if(!sfx_.complete_using(true,true)){pending_.reset();pending_view_={};failed_=true;return false;}
        state_=std::move(*pending_);pending_.reset();committed_view_=pending_view_;pending_view_={};++epoch_->commits;return true;
    }
    SoundPcmSource sfx_;StartupMusicLibrary music_;MusicState state_;std::optional<MusicState> pending_;
    StartupSoundInputView pending_view_,committed_view_;std::shared_ptr<startup_audio_pcm_detail::Epoch> epoch_;bool loaded_=false,failed_=false;
};
} // namespace gf
