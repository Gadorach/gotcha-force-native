#pragma once
// Source-weighted native SFX producer for the EXISTING shared PcmMixer/PcmQueue.
// Source32k frames buffered across arbitrary device block sizes. Pure callback
// provider supplies event-due/controls; accepted native state commits only when
// Legacy callers promise Complete(true) follows actual queue submission; tracked
// inputs additionally require the private genuine queue-completion path.
// Neither accepted source state nor a tracked receipt is speaker ACK.
#include <optional>
#include "engine/audio/pcm_queue.hpp"
#include "engine/audio/pcm_wide_rate.hpp"
#include "engine/gameplay/sound_cpu_tick.hpp"
#include "engine/gameplay/sound_pcm_boundaries.hpp"
namespace gf {
struct SoundPcmFrameInputs {
    StartupSoundRing ring;SoundCpuTickObservation event;
    SoundVoiceControlObservation controls;std::vector<SoundVoiceBankGainObservation> banks;
};
class SoundPcmFrameProvider {
public:
    virtual ~SoundPcmFrameProvider()=default;
    // Pure/replayable DATA lookup, no device/queue or mutable game-state writes.
    virtual bool inputs(const SoundCpuControlState&,SoundPcmFrameInputs*,std::string*)const=0;
};
struct SoundPcmSourceSnapshot {
    u64 admitted_device_frames=0,generated_control_frames=0;unsigned buffered_position=160;
    PcmWideRateState conversion;SoundCpuBackendSnapshot backend;u64 event_pass=0;
    bool failed=false,pending=false;
    u32 event_boundary=0,boundary_ring_read=0,boundary_ring_write=0;
    u32 boundary_forwarded=0,boundary_skipped=0,boundary_dropped=0;
    SoundPcmInputMode input_mode=SoundPcmInputMode::Unselected;
};
class SoundPcmSource {
    friend class StartupAudioPcmSource;
    friend class SceneAudioPcmSource;
public:
    bool load(SoundVoiceLibrary library,SoundVoiceProfile voice,SoundEnvelopeProfile envelope,u32 target,std::string* error){
        if(loaded_||failed_||pending_){if(error)*error="SFX PCM source: live/reused owner reload refused; use a fresh source";return false;}
        if(!library.ready()||!sound_voice_profile_valid(voice)||!envelope.ready()||!state_.rate.load(target)){if(error)*error="SFX PCM source: library/profile/device rate";return false;}
        library_=std::move(library);voice_=voice;envelope_=envelope;loaded_=true;if(error)error->clear();return true;
    }
    bool prepare(unsigned frames,PcmMixer& mixer,const SoundPcmFrameProvider& provider,std::string* error){
        auto next=[&](auto& candidate,SoundCpuDropFrame* pcm,std::string* why){
            SoundPcmFrameInputs inputs;if(!provider.inputs(candidate.cpu,&inputs,why))return false;SoundCpuTickPlan tick;
            if(!sound_cpu_tick(library_,voice_,envelope_,candidate.cpu,inputs.ring,inputs.event,inputs.controls,inputs.banks,&tick,why))return false;
            candidate.cpu=std::move(tick.state);*pcm=std::move(tick.pcm);return true;
        };
        return prepare_using(frames,mixer,SoundPcmInputMode::LegacyFrameInputs,next,error);
    }
    bool prepare_boundaries(unsigned frames,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,std::string* error){
        return prepare_boundary_mode(frames,mixer,provider,SoundPcmInputMode::EventBoundaries,{},error);
    }
    std::shared_ptr<SoundPcmBoundaryBinding> create_boundary_binding(std::string* error){
        if(!loaded_||failed_||pending_||binding_issued_||state_.mode!=SoundPcmInputMode::Unselected||admitted_){if(error)*error="SFX tracked boundary binding: fresh loaded source/one issuance required";return {};}
        epoch_=std::make_shared<sound_pcm_boundary_detail::SourceEpoch>();auto binding=std::make_shared<SoundPcmBoundaryBinding>();binding->source_=epoch_;binding_issued_=true;binding_=binding;if(error)error->clear();return binding;
    }
    bool prepare_tracked_boundaries(unsigned frames,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,std::string* error){
        return prepare_tracked_with_cuts(frames,mixer,provider,nullptr,error);
    }
private:
    bool prepare_tracked_with_cuts(unsigned frames,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,std::vector<SoundPcmBoundaryCut>* cuts,std::string* error){
        if(cuts)cuts->clear();
        if(!loaded_||failed_||pending_){if(error)*error="SFX PCM source: unloaded/failed/already pending";return false;}
        const auto binding=provider.binding();
        if(!binding||!binding->live()||!epoch_||binding!=binding_.lock()||binding->source_.lock()!=epoch_||epoch_->commits!=state_.tracked_sequence){failed_=true;if(error)*error="SFX tracked boundaries: source/producer binding or committed sequence mismatch";return false;}
        return prepare_boundary_mode(frames,mixer,provider,SoundPcmInputMode::TrackedEventBoundaries,binding,error,cuts);
    }
private:
    bool prepare_boundary_mode(unsigned frames,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,SoundPcmInputMode mode,std::shared_ptr<const SoundPcmBoundaryBinding> binding,std::string* error,std::vector<SoundPcmBoundaryCut>* cuts=nullptr){
        if(!loaded_||failed_||pending_){if(error)*error="SFX PCM source: unloaded/failed/already pending";return false;}
        if(!provider.valid(error)){failed_=true;return false;}
        const u32 available=provider.available(state_.cpu);
        if(available<state_.boundary||available>8192){failed_=true;if(error)*error="SFX PCM boundaries: stale prefix/native boundary budget";return false;}
        unsigned producing_offset=0;std::vector<SoundPcmBoundaryCut> candidate_cuts;
        auto next=[&](auto& candidate,SoundCpuDropFrame* pcm,std::string* why){
            if(binding)candidate.binding=binding;
            const u32 limit=provider.available(candidate.cpu);
            if(limit<candidate.boundary||limit>8192){if(why)*why="SFX PCM boundaries: stale prefix/native boundary budget";return false;}
            while(candidate.boundary<limit){SoundPcmEventBoundary input;const u32 ordinal=candidate.boundary+1;
                if(!provider.boundary(ordinal,&input,why))return false;
                if(input.ordinal!=ordinal){if(why)*why="SFX PCM boundaries: consecutive explicit boundary required";return false;}
                StartupSoundTransferPlan transport;if(!startup_sound_ring_transfer(input.staging,candidate.ring,&transport,why))return false;
                SoundCpuEventPlan event;if(!sound_cpu_event_step(library_,voice_,candidate.cpu,transport.ring,input.mode,&event,why))return false;
                candidate.cpu=std::move(event.state);candidate.ring=std::move(event.ring);candidate.boundary=ordinal;
                if(cuts)candidate_cuts.push_back({producing_offset,ordinal});
                candidate.forwarded+=transport.forwarded;candidate.skipped+=transport.skipped;candidate.dropped+=transport.dropped;
            }
            SoundVoiceControlObservation controls;std::vector<SoundVoiceBankGainObservation> banks;
            if(!provider.controls(candidate.cpu,&controls,&banks,why))return false;
            SoundCpuControlPlan control;if(!sound_cpu_control_frame(voice_,envelope_,candidate.cpu,controls,banks,&control,why))return false;
            candidate.cpu=std::move(control.state);*pcm=std::move(control.pcm);return true;
        };
        if(!prepare_using(frames,mixer,mode,next,error,&producing_offset))return false;
        if(cuts)*cuts=std::move(candidate_cuts);
        return true;
    }
    template<class Next> bool prepare_using(unsigned frames,PcmMixer& mixer,SoundPcmInputMode mode,Next next,std::string* error,unsigned* producing_offset=nullptr){
        if(!loaded_||failed_||pending_){if(error)*error="SFX PCM source: unloaded/failed/already pending";return false;}
        if(state_.mode!=SoundPcmInputMode::Unselected&&state_.mode!=mode){failed_=true;if(error)*error="SFX PCM source: explicit input modes cannot be mixed";return false;}
        auto candidate=state_;std::vector<s64> stereo;
        candidate.mode=mode;
        auto pull=[&](std::array<s32,2>* sample,unsigned offset,std::string* why){
            if(producing_offset)*producing_offset=offset;
            if(candidate.position==160){SoundCpuDropFrame pcm;if(!next(candidate,&pcm,why))return false;
                // Explicit qualified dry L/R subset. Never fold unimplemented
                // aux/surround returns into dry or silently drop nonzero lanes.
                for(unsigned lane=2;lane<9;++lane)for(auto value:pcm.lanes[lane])if(value){if(why)*why="SFX PCM source: nonzero aux/surround requires effect/output consumer";return false;}
                candidate.buffer=std::move(pcm.lanes);candidate.position=0;
            }
            (*sample)[0]=candidate.buffer[0][candidate.position];(*sample)[1]=candidate.buffer[1][candidate.position];++candidate.position;return true;
        };
        if(!candidate.rate.render_positioned(frames,pull,&stereo,error)||!mixer.add_source_weighted_sfx(stereo)){failed_=true;if(error&&error->empty())*error="SFX PCM source: shared mixer refused weighted wide bus";return false;}
        pending_=std::move(candidate);pending_frames_=frames;return true;
    }
public:
    bool complete(bool accepted){
        return complete_using(accepted,false);
    }
private:
    bool pending_commit_valid()const{
        if(!pending_||failed_||admitted_>std::numeric_limits<u64>::max()-pending_frames_)return false;
        if(pending_->mode!=SoundPcmInputMode::TrackedEventBoundaries)return true;
        const auto binding=pending_->binding.lock();
        return epoch_&&binding&&binding->live()&&epoch_->commits==state_.tracked_sequence&&epoch_->commits!=std::numeric_limits<u64>::max();
    }
    bool complete_using(bool accepted,bool tracked_queue_completion){
        if(!pending_)return !failed_;
        // A public boolean (even after a caller's manual sink submission) is not
        // tracked delivery authority. Only our private callback in the real queue
        // pump can commit tracked state. Legacy/untracked semantics stay intact.
        if(pending_->mode==SoundPcmInputMode::TrackedEventBoundaries&&!tracked_queue_completion)accepted=false;
        if(!accepted){pending_.reset();pending_frames_=0;failed_=true;return false;}
        if(admitted_>std::numeric_limits<u64>::max()-pending_frames_){pending_.reset();pending_frames_=0;failed_=true;return false;}
        if(pending_->mode==SoundPcmInputMode::TrackedEventBoundaries){const auto binding=pending_->binding.lock();if(!epoch_||!binding||!binding->live()||epoch_->commits!=state_.tracked_sequence||epoch_->commits==std::numeric_limits<u64>::max()){pending_.reset();pending_frames_=0;failed_=true;return false;}pending_->tracked_sequence=++epoch_->commits;}
        state_=std::move(*pending_);pending_.reset();admitted_+=pending_frames_;pending_frames_=0;return true;
    }
public:
    // Convenient source-only pump. BGM/SFX host uses the same prepare/complete
    // hooks in ONE shared queue pump, not two independent sink owners.
    bool pump(PcmQueue& queue,PcmMixer& mixer,const SoundPcmFrameProvider& provider,std::string* error){
        if(!loaded_||failed_||pending_)return false;
        return queue.pump(mixer,[&](unsigned n,PcmMixer& mix){return prepare(n,mix,provider,error);},[&](bool accepted){return complete(accepted);});
    }
    bool pump_boundaries(PcmQueue& queue,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,std::string* error){
        if(!loaded_||failed_||pending_)return false;
        return queue.pump(mixer,[&](unsigned n,PcmMixer& mix){return prepare_boundaries(n,mix,provider,error);},[&](bool accepted){return complete(accepted);});
    }
    bool pump_tracked_boundaries(PcmQueue& queue,PcmMixer& mixer,const SoundPcmBoundaryProvider& provider,std::string* error){
        if(!loaded_||failed_||pending_)return false;
        return queue.pump(mixer,[&](unsigned n,PcmMixer& mix){return prepare_tracked_boundaries(n,mix,provider,error);},[&](bool accepted){return complete_using(accepted,true);});
    }
    SoundPcmBoundaryReceipt boundary_receipt()const{
        SoundPcmBoundaryReceipt receipt;if(!epoch_||state_.mode!=SoundPcmInputMode::TrackedEventBoundaries||!state_.tracked_sequence||!admitted_)return receipt;
        receipt.source_=epoch_;receipt.binding_=state_.binding;receipt.boundary_=state_.boundary;receipt.device_frames_=admitted_;receipt.control_frames_=state_.cpu.parameters.lifecycle.callback;receipt.sequence_=state_.tracked_sequence;return receipt;
    }
    SoundPcmSourceSnapshot snapshot()const{const auto& life=state_.cpu.parameters.lifecycle;return {admitted_,life.callback,state_.position,state_.rate.state(),state_.cpu.backend.snapshot(),life.event_pass,failed_,bool(pending_),state_.boundary,state_.ring.read,state_.ring.write,state_.forwarded,state_.skipped,state_.dropped,state_.mode};}
    bool failed()const{return failed_;}
private:
    struct State {SoundCpuControlState cpu;PcmWideRate rate;std::array<std::array<s32,160>,9> buffer{};unsigned position=160;SoundPcmInputMode mode=SoundPcmInputMode::Unselected;u32 boundary=0,forwarded=0,skipped=0,dropped=0;StartupSoundRing ring;std::weak_ptr<const SoundPcmBoundaryBinding> binding;u64 tracked_sequence=0;};
    SoundVoiceLibrary library_;SoundVoiceProfile voice_;SoundEnvelopeProfile envelope_;
    State state_;std::optional<State> pending_;u64 admitted_=0;unsigned pending_frames_=0;bool loaded_=false,failed_=false;
    std::shared_ptr<sound_pcm_boundary_detail::SourceEpoch> epoch_;std::weak_ptr<const SoundPcmBoundaryBinding> binding_;bool binding_issued_=false;
};
} // namespace gf
