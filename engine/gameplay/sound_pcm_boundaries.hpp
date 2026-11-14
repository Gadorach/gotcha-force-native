#pragma once
// Explicit immutable source-main-iteration boundaries, distinct from160-sample
// audio control frames. No fixed game FPS, event/control ratio or OS tick invented.
#include "engine/gameplay/sound_cpu_tick.hpp"
#include "engine/gameplay/startup_sound_dispatch.hpp"
#include <memory>
namespace gf {
// Ordered source-event progress at a native device-output production cut. A cut
// may be at block end because the existing converter retains two-sample lookahead.
// These are speculative DATA, not delivery receipts or original IRQ/latency proof.
struct SoundPcmBoundaryCut {unsigned output_offset=0;u32 boundary=0;};
enum class SoundPcmInputMode:u8 {Unselected,LegacyFrameInputs,EventBoundaries,TrackedEventBoundaries};
namespace sound_pcm_boundary_detail {
struct SourceEpoch {u64 commits=0;};
struct ProducerEpoch {};
}
class SoundPcmSource;
class SoundPcmBoundaryBinding {
public:
    SoundPcmBoundaryBinding()=default;
    SoundPcmBoundaryBinding(const SoundPcmBoundaryBinding&)=delete;
    SoundPcmBoundaryBinding& operator=(const SoundPcmBoundaryBinding&)=delete;
    SoundPcmBoundaryBinding(SoundPcmBoundaryBinding&&)=delete;
    SoundPcmBoundaryBinding& operator=(SoundPcmBoundaryBinding&&)=delete;
    bool live()const{return claimed_&&!source_.expired()&&!producer_.expired();}
    bool claim(const std::shared_ptr<const sound_pcm_boundary_detail::ProducerEpoch>& producer,std::string* error){
        if(claimed_||source_.expired()||!producer){if(error)*error="SFX boundary binding: one live producer claim required";return false;}
        claimed_=true;producer_=producer;if(error)error->clear();return true;
    }
private:
    friend class SoundPcmSource;
    std::weak_ptr<sound_pcm_boundary_detail::SourceEpoch> source_;
    std::weak_ptr<const sound_pcm_boundary_detail::ProducerEpoch> producer_;bool claimed_=false;
};
class SoundPcmBoundaryReceipt {
public:
    bool valid()const{const auto source=source_.lock();const auto binding=binding_.lock();return source&&binding&&binding->live()&&sequence_&&sequence_<=source->commits&&device_frames_&&control_frames_;}
    u32 boundary()const{return boundary_;}u64 admitted_device_frames()const{return device_frames_;}u64 control_frames()const{return control_frames_;}u64 sequence()const{return sequence_;}
    bool matches(const std::shared_ptr<const SoundPcmBoundaryBinding>& binding)const{return valid()&&binding&&binding==binding_.lock();}
    bool same_source(const SoundPcmBoundaryReceipt& other)const{const auto source=source_.lock();return source&&source==other.source_.lock();}
private:
    friend class SoundPcmSource;
    std::weak_ptr<sound_pcm_boundary_detail::SourceEpoch> source_;
    std::weak_ptr<const SoundPcmBoundaryBinding> binding_;
    u32 boundary_=0;u64 device_frames_=0,control_frames_=0,sequence_=0;
};
struct SoundPcmEventBoundary {
    u32 ordinal=0;StartupSoundCommandQueue staging;
    SoundVoiceLifecycleEventObservation mode;
};
class SoundPcmBoundaryProvider {
public:
    virtual ~SoundPcmBoundaryProvider()=default;
    // Pure/replayable DATA lookup. No controller/save/manager acknowledgement or
    // event allocation here. Prefix publication/scheduling belongs to its owner;
    // snapshots must not change while the queue prepares a speculative block.
    virtual bool valid(std::string*)const=0;
    virtual u32 available(const SoundCpuControlState&)const=0;
    virtual bool boundary(u32 ordinal,SoundPcmEventBoundary*,std::string*)const=0;
    virtual bool controls(const SoundCpuControlState&,SoundVoiceControlObservation*,std::vector<SoundVoiceBankGainObservation>*,std::string*)const=0;
    // Only explicit tracked entries consume this binding. Existing untracked
    // providers keep their old behavior and cannot produce delivery receipts.
    virtual std::shared_ptr<const SoundPcmBoundaryBinding> binding()const{return {};}
};
} // namespace gf
