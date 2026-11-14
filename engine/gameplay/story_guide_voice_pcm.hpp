#pragma once
#include <memory>
#include <optional>
#include "engine/assets/story_guide_voice.hpp"
#include "engine/audio/source_adx_sdk_gain.hpp"
#include "engine/audio/pcm_player.hpp"
#include "engine/audio/pcm_wide_rate.hpp"
#include "engine/audio/pcm_queue.hpp"
namespace gf {
class SceneAudioPcmSource;
class StoryGuideVoiceController;
namespace guide_voice_pcm_detail {struct Scope {bool live=true;std::array<u64,2> generation{};};}
class StoryGuideVoiceGeneration {
public:
    unsigned slot()const{return slot_;}u64 generation()const{return generation_;}
private:
    friend class StoryGuideVoicePcmSource;std::weak_ptr<guide_voice_pcm_detail::Scope> scope_;unsigned slot_=2;u64 generation_=0;
};
enum class StoryGuideVoicePcmPhase:u8 {Idle,Primed,Streaming,Draining,Ended};
//Native scheduling/inspection DATA, not command-ring or playback authority.
struct StoryGuideVoiceGainSchedule {unsigned slot=2;u64 generation=0,next_control_serial=0;s32 requested_units=0;bool changed=false,pending=false;bool requires_audio_consumer()const{return true;}};
struct StoryGuideVoiceRawFrame {unsigned slot=2;u64 generation=0,frame_index=0;u32 key=0;std::array<s16,160> mono{};SourceAdxSdkGainPacket gain;};
struct StoryGuideVoicePcmSnapshot {
    StoryGuideVoicePcmPhase phase=StoryGuideVoicePcmPhase::Idle;u64 generation=0;u32 key=0;bool paused=false;u8 pause72=0;s32 sdk_volume=0;PcmPlayerCursor decoder;PcmWideRateState conversion;u32 buffered_position=160;u64 output_remaining=0,voice_frames=0,bus_frames=0,control_frames=0;bool failed=false,pending=false;
    //Source-compatible native status label ONLY, not original SDK timing/heard.
    s32 native_status()const{return failed?-1:phase==StoryGuideVoicePcmPhase::Idle?0:phase==StoryGuideVoicePcmPhase::Primed?1:phase==StoryGuideVoicePcmPhase::Ended?5:3;}
    u64 suspended_frames=0,resumed_frames=0;
    u64 gain_control_serial=0;u16 materialized_gain_target=0;bool volume_pending=false,gain_settling=false;
};
//Actual native decoded-resource/generation/PCM owner for two fixed guide voices.
//One borrowed serialized PcmQueue must outlive this owner and not be copied or
//replaced. Original AX SRC/idle clock/status timing and guide VM remain separate.
class StoryGuideVoicePcmSource {
    friend class SceneAudioPcmSource;
    friend class StoryGuideVoiceController;
public:
    StoryGuideVoicePcmSource()=default;StoryGuideVoicePcmSource(const StoryGuideVoicePcmSource&)=delete;StoryGuideVoicePcmSource& operator=(const StoryGuideVoicePcmSource&)=delete;
    bool load(StoryGuideVoiceLibrary library,const SourceAdxSdkGainProfile& gain,SourceAdxSdkOutput mode,u32 rate,PcmQueue& queue,std::string* error){
        if(loaded_||failed_||!queue.guide_voice_owner_.expired()||!library.ready()||!gain.ready()||unsigned(mode)>1||rate<8000||rate>192000)return fail(error,"fresh immutable resources/rate/mode/exclusive queue owner");
        State next;for(unsigned i=0;i<2;++i){const auto* audio=library.audio(60+i);PcmVoiceTailObservation tail;if(!audio||audio->channels!=1||audio->loop||!pcm_player_audio_valid(*audio,32000)||!queue.observe_voice_slot(i,&tail)||tail.slot_frames||(!tail.output_available&&queue.journal_frames())||!next.slots[i].gain.configure(gain,mode)||!next.slots[i].rate.load(rate))return fail(error,"native mono handles/owned queue/gain initialization");}
        library_=std::move(library);state_=std::move(next);rate_=rate;mode_=mode;queue_=&queue;scope_=std::make_shared<guide_voice_pcm_detail::Scope>();queue.guide_voice_owner_=scope_;loaded_=true;if(error)error->clear();return true;
    }
    bool ready()const{return loaded_&&!failed_&&scope_&&scope_->live&&queue_&&!queue_->failed()&&queue_->guide_voice_owner_.lock()==scope_;}u32 output_rate()const{return rate_;}
    StoryGuideVoiceGeneration handle(unsigned slot)const{StoryGuideVoiceGeneration h;if(ready()&&slot<2){h.scope_=scope_;h.slot_=slot;h.generation_=state_.slots[slot].generation;}return h;}
    bool current(const StoryGuideVoiceGeneration& h)const{return ready()&&h.slot_<2&&h.scope_.lock()==scope_&&h.generation_==state_.slots[h.slot_].generation&&scope_->generation[h.slot_]==h.generation_;}
    bool start(unsigned slot,unsigned key,StoryGuideVoiceGeneration* out,std::string* error){
        if(out)*out={};
        if(!out||!ready()||pending_||pumping_||controlling_||slot>=2||(key!=60&&key!=61))return false;
        auto& s=state_.slots[slot];if(s.generation==std::numeric_limits<u64>::max())return fail(error,"generation overflow");if(!cut(slot,error))return false;const auto* audio=library_.audio(key);if(!audio)return fail(error,"native source key");const u64 source=(u64(audio->frames)*32000+audio->rate-1)/audio->rate;s.phase=StoryGuideVoicePcmPhase::Primed;s.key=key;s.cursor={};s.cursor.playing=true;s.position=160;s.buffer={};s.output_remaining=(source*rate_+31999)/32000;s.voice_frames=s.bus_frames=s.control_frames=0;s.pause72=0;s.paused=false;if(!s.rate.load(rate_))return fail(error,"native conversion reset");
        s.suspension.clear();s.resume_at=0;s.resumed_frames=0;s.raw_buffer={};s.gain_packet={};++s.generation;scope_->generation[slot]=s.generation;*out=handle(slot);if(error)error->clear();return true;
    }
    bool stop(const StoryGuideVoiceGeneration& h,std::string* error){
        if(!current(h)||pending_||pumping_||controlling_)return false;
        auto& s=state_.slots[h.slot_];if(s.generation==std::numeric_limits<u64>::max())return fail(error,"generation overflow");if(!cut(h.slot_,error))return false;s.phase=StoryGuideVoicePcmPhase::Idle;s.key=0;s.cursor={};s.position=160;s.buffer={};s.output_remaining=0;s.paused=false;s.pause72=0;if(!s.rate.load(rate_))return fail(error,"stop conversion reset");
        s.suspension.clear();s.resume_at=0;s.raw_buffer={};s.gain_packet={};++s.generation;scope_->generation[h.slot_]=s.generation;if(error)error->clear();return true;
    }
    bool set_volume(const StoryGuideVoiceGeneration& h,s32 units,std::string* error){
        if(!current(h)||pending_||pumping_||controlling_)return false;
        auto& s=state_.slots[h.slot_];const s32 clamped=units>0?0:units<-999?-999:units;if(clamped==s.gain.requested_volume())return true;
        //Weighted queued/buffered data cannot be inverted/reweighted safely.
        //The guide's fixed setting is supported; live active retargeting refuses.
        if(s.phase!=StoryGuideVoicePcmPhase::Idle&&s.phase!=StoryGuideVoicePcmPhase::Ended){if(error)*error="guide voice PCM: active gain retargeting requires raw-tail ownership";return false;}return s.gain.request_volume(units);
    }
    //Explicit native future-only policy. Already materialized buffers, converter
    //lookahead, queued/suspended PCM are immutable. Latest request is consumed by
    //the NEXT newly generated160-source-sample frame, not by this setter/UI tick.
    //No promise that the current clip has another frame or that it will be heard.
    bool schedule_volume_next_frame(const StoryGuideVoiceGeneration& h,s32 units,StoryGuideVoiceGainSchedule* out,std::string* error){
        if(!out||!current(h)||pending_||pumping_||controlling_)return false;
        auto& s=state_.slots[h.slot_];if(s.gain.control_serial()==std::numeric_limits<u64>::max())return fail(error,"gain control serial overflow");
        const s32 clamped=units>0?0:units<-999?-999:units;const bool changed=clamped!=s.gain.requested_volume();if(!s.gain.request_volume(units))return fail(error,"live native gain request");
        *out={h.slot_,s.generation,s.gain.control_serial()+1,s.gain.requested_volume(),changed,s.gain.dirty()};if(error)error->clear();return true;
    }
    //Copies the last genuinely committed raw native source frame and its exact
    //materialized gain packet. These are diagnostics/input DATA, never a receipt
    //for all remaining queued speech or a capability to reweight a sink tail.
    bool copy_committed_raw_frame(const StoryGuideVoiceGeneration& h,StoryGuideVoiceRawFrame* out)const{
        if(!out||!current(h)||pending_||pumping_||controlling_)return false;
        const auto& s=state_.slots[h.slot_];if(!s.gain_packet.control_serial)return false;*out={h.slot_,s.generation,s.control_frames,s.key,s.raw_buffer,s.gain_packet};return true;
    }
    bool pause(const StoryGuideVoiceGeneration& h,s32 value,std::string* error){
        if(!current(h)||pending_||pumping_||controlling_)return false;
        auto& s=state_.slots[h.slot_];if(value==std::bit_cast<s8>(s.pause72))return true;if(value==1){Boundary guard(controlling_);PcmVoiceTailObservation tail;if(!queue_->observe_voice_slot(h.slot_,&tail)||!current(h)||(!tail.output_available&&queue_->journal_frames())||tail.slot_frames){if(error)*error="guide voice PCM: queued pause requires exact raw/retained-tail suspension";return false;}}s.pause72=u8(value);s.paused=value==1;if(error)error->clear();return true;
    }
    //Explicit native suspension of already-weighted unplayed speech. No decoder,
    //gain or source-clock rewind; changing gain while this data exists refuses.
    //The old pause entry keeps its queued-tail refusal for existing callers.
    bool pause_retaining_queued(const StoryGuideVoiceGeneration& h,s32 value,std::string* error){
        if(!current(h)||pending_||pumping_||controlling_)return false;
        if(value!=1)return pause(h,value,error);
        auto& slot=state_.slots[h.slot_];if(value==std::bit_cast<s8>(slot.pause72))return true;
        Boundary guard(controlling_);PcmVoiceTailObservation tail;if(!queue_->observe_voice_slot(h.slot_,&tail)||!current(h)||(!tail.output_available&&queue_->journal_frames())){if(error)*error="guide voice PCM: unknown unplayed tail cannot be suspended";return false;}
        const auto held=slot.suspension.size()/2-slot.resume_at;
        //Native bounded retention budget, not an original SDK buffer capacity.
        if(held+tail.queued_frames>4096){if(error)*error="guide voice PCM: bounded suspended tail capacity";return false;}
        auto next=slot;std::vector<s64> combined;combined.reserve(std::size_t(held+tail.queued_frames)*2);
        if(tail.slot_frames){PcmSuspendedVoiceTail captured;if(!queue_->suspend_voice_slot(h.slot_,&captured)){if(queue_->failed())return fail(error,"failed/uncertain voice suspension; no retry");if(error)*error="guide voice PCM: unsupported voice suspension";return false;}if(!current(h))return fail(error,"owner invalidated during voice suspension");combined=std::move(captured.stereo);}
        combined.insert(combined.end(),slot.suspension.begin()+std::size_t(slot.resume_at)*2,slot.suspension.end());next.suspension=std::move(combined);next.resume_at=0;next.paused=true;next.pause72=1;slot=std::move(next);if(error)error->clear();return true;
    }
    bool observe_queue(std::string* error){
        if(!ready()||pending_||pumping_||controlling_)return false;
        Boundary guard(controlling_);for(unsigned i=0;i<2;++i){auto& s=state_.slots[i];if(s.phase!=StoryGuideVoicePcmPhase::Draining||s.resume_at<s.suspension.size()/2)continue;PcmVoiceTailObservation tail;if(!queue_->observe_voice_slot(i,&tail)||!ready())return fail(error,"owned tracked voice tail observation");if(tail.output_available&&!tail.slot_frames)s.phase=StoryGuideVoicePcmPhase::Ended;}if(error)error->clear();return true;
    }
    bool pump(PcmMixer& mixer,std::string* error){
        if(!ready()||pending_||pumping_||controlling_)return false;
        bool ok=false;{Boundary guard(pumping_);ok=queue_->pump(mixer,[&](unsigned n,PcmMixer& m){return prepare_pcm(n,m,error);},[&](bool accepted){if(!accepted||!pending_valid())return reject_pending(error);accept_pending();return true;});}if(!ok)return reject_pending(error);return observe_queue(error);
    }
    StoryGuideVoicePcmSnapshot snapshot(unsigned slot)const{StoryGuideVoicePcmSnapshot p;if(slot>=2){p.failed=true;return p;}const auto& s=state_.slots[slot];p.phase=s.phase;p.generation=s.generation;p.key=s.key;p.paused=s.paused;p.pause72=s.pause72;p.sdk_volume=s.gain.requested_volume();p.decoder=s.cursor;p.conversion=s.rate.state();p.buffered_position=s.position;p.output_remaining=s.output_remaining;p.voice_frames=s.voice_frames;p.bus_frames=s.bus_frames;p.control_frames=s.control_frames;p.failed=!ready();p.pending=bool(pending_);p.suspended_frames=s.suspension.size()/2-s.resume_at;p.resumed_frames=s.resumed_frames;p.gain_control_serial=s.gain.control_serial();p.materialized_gain_target=s.gain_packet.target;p.volume_pending=s.gain.dirty();p.gain_settling=s.gain.settling();return p;}
    void close(){if(scope_)scope_->live=false;scope_.reset();pending_.reset();failed_=true;}
private:
    struct Boundary {bool& busy;explicit Boundary(bool& b):busy(b){busy=true;}~Boundary(){busy=false;}Boundary(const Boundary&)=delete;Boundary& operator=(const Boundary&)=delete;};
    struct Slot {StoryGuideVoicePcmPhase phase=StoryGuideVoicePcmPhase::Idle;u64 generation=0;u32 key=0;bool paused=false;u8 pause72=0;PcmPlayerCursor cursor;PcmWideRate rate;SourceAdxSdkGainControl gain;std::array<std::array<s32,2>,160> buffer{};unsigned position=160;u64 output_remaining=0,voice_frames=0,bus_frames=0,control_frames=0;std::vector<s64> suspension;unsigned resume_at=0;u64 resumed_frames=0;std::array<s16,160> raw_buffer{};SourceAdxSdkGainPacket gain_packet;};
    struct State {std::array<Slot,2> slots;};State state_;std::optional<State> pending_;std::array<u64,2> pending_generation_{};u32 rate_=0;SourceAdxSdkOutput mode_=SourceAdxSdkOutput::Stereo;StoryGuideVoiceLibrary library_;PcmQueue* queue_=nullptr;std::shared_ptr<guide_voice_pcm_detail::Scope> scope_;bool loaded_=false,failed_=false,pumping_=false,controlling_=false;
    std::weak_ptr<const void> guide_control_owner_;
    bool cut(unsigned slot,std::string* error){Boundary guard(controlling_);PcmVoiceTailObservation tail;if(!queue_->observe_voice_slot(slot,&tail)||!ready()||(!tail.output_available&&queue_->journal_frames())){if(error)*error="guide voice PCM: unavailable/untracked tail cannot be silently adopted";return false;}if(!queue_->cut_voice_slot(slot)){if(queue_->failed())return fail(error,"failed/uncertain voice cut; no retry");if(error)*error="guide voice PCM: unsupported voice cut";return false;}if(!ready())return fail(error,"owner invalidated during native voice cut");return true;}
    bool prepare_pcm(unsigned frames,PcmMixer& mixer,std::string* error){
        if(!ready()||pending_||!frames||frames>4096||mixer.mono_policy()!=(mode_==SourceAdxSdkOutput::Mono))return fail(error,"serialized native PCM frames/output mode");
        auto next=state_;
        for(unsigned slot=0;slot<2;++slot){
            auto& s=next.slots[slot];pending_generation_[slot]=s.generation;if(s.paused)continue;
            const unsigned resumed=unsigned(std::min<std::size_t>(frames,s.suspension.size()/2-s.resume_at));
            const unsigned count=(s.phase==StoryGuideVoicePcmPhase::Primed||s.phase==StoryGuideVoicePcmPhase::Streaming)?unsigned(std::min<u64>(frames-resumed,s.output_remaining)):0;
            if(!resumed&&!count)continue;
            std::vector<s64> pcm(std::size_t(frames)*2,0);if(resumed)std::copy_n(s.suspension.begin()+std::size_t(s.resume_at)*2,std::size_t(resumed)*2,pcm.begin());
            if(count){const auto* audio=library_.audio(s.key);if(!audio)return fail(error,"owned decoded mono clip");std::vector<s64> fresh;if(!render_fresh(s,*audio,count,&fresh,error))return fail(error,"native rate/source gain frame");std::copy(fresh.begin(),fresh.end(),pcm.begin()+std::size_t(resumed)*2);}
            if(!mixer.add_source_weighted_voice_slot(slot,pcm)||s.voice_frames>std::numeric_limits<u64>::max()-count||s.bus_frames>std::numeric_limits<u64>::max()-frames||s.resumed_frames>std::numeric_limits<u64>::max()-resumed)return fail(error,"bounded weighted voice block");
            s.resume_at+=resumed;s.resumed_frames+=resumed;if(s.resume_at==s.suspension.size()/2){s.suspension.clear();s.resume_at=0;}
            s.output_remaining-=count;s.voice_frames+=count;s.bus_frames+=frames;s.phase=s.output_remaining?StoryGuideVoicePcmPhase::Streaming:StoryGuideVoicePcmPhase::Draining;
        }
        pending_=std::move(next);return true;
    }
    bool render_fresh(Slot& s,const AdxAudio& audio,unsigned count,std::vector<s64>* pcm,std::string* error){
        auto pull=[&](std::array<s32,2>* value,std::string* why){if(s.position==160){std::array<s16,160> mono{};if(s.cursor.playing){std::vector<s16> input;if(!pcm_player_render_cursor(audio,32000,&s.cursor,160,&input))return false;for(unsigned i=0;i<160;++i)mono[i]=input[i*2];}SourceAdxSdkGainPacket packet;std::vector<s64> weighted;if(!s.gain.update_control(&packet)||!source_adx_sdk_gain_render(packet,mono,&weighted))return false;s.raw_buffer=mono;s.gain_packet=packet;for(unsigned i=0;i<160;++i)for(unsigned c=0;c<2;++c)s.buffer[i][c]=s32(weighted[i*2+c]/65536);s.position=0;if(s.control_frames==std::numeric_limits<u64>::max())return false;++s.control_frames;}*value=s.buffer[s.position++];if(why)why->clear();return true;};return s.rate.render(count,pull,pcm,error);
    }
    bool pending_valid()const{if(!ready()||!pending_)return false;for(unsigned i=0;i<2;++i)if(pending_generation_[i]!=state_.slots[i].generation||scope_->generation[i]!=pending_generation_[i])return false;return true;}
    void accept_pending(){state_=std::move(*pending_);pending_.reset();}
    bool reject_pending(std::string* error){pending_.reset();return fail(error,"rejected/uncertain joint PCM block; no replay");}
    bool fail(std::string* error,const char* why){failed_=true;if(scope_)scope_->live=false;if(error)*error=std::string("guide voice PCM: ")+why;return false;}
};
} //namespace gf
