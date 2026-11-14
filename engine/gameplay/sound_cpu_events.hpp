#pragma once
// Automatic native acquisition at the EXISTING shared region traversal seam.
// Whole ring/event/software/native/cache candidate; no declared acquisition input.
// Control/settings producers and shared PCM queue/manager ACK remain separate.
#include "engine/gameplay/sound_cpu_control_frame.hpp"
namespace gf {
struct SoundCpuEventPlan {
    SoundCpuControlState state;StartupSoundRing ring;
    std::vector<SoundVoiceLifecycleAcquisition> actual_acquisitions;
    unsigned initialized=0,steals=0,unlinked_children=0,released_events=0;
    bool reentrant_skip=false,requires_bus_queue_consumer=true;
};
namespace sound_cpu_event_detail {
class RegionAcquire final:public SoundVoiceAcquireProvider {
public:
    RegionAcquire(SoundCpuBackend& backend,const SoundVoiceLifecycleState& source,u16 event,std::vector<SoundVoiceLifecycleAcquisition>& receipts)
        :backend_(backend),source_(source),event_(event),receipts_(receipts),generation_(source.generation),preview_(source.voice_generation){}
    u64 generation()const{return generation_;}
    bool acquire(const SoundVoiceProfile& profile,const SoundVoiceAcquireRequest& request,SoundVoiceAcquireReply* out,std::string* error)override{
        if(out)*out={};
        auto fail=[&](const char* why){if(error)*error=std::string("native region acquire: ")+why;return false;};
        if(!out||request.event!=event_||request.software_slot>=64||generation_==std::numeric_limits<u64>::max())return fail("request/context/generation budget");
        SoundVoiceSeed initialized;bool called=false;SoundCpuAcquisition acquired;
        const SoundVoiceLifecycleIdentity owner{event_,request.software_slot,source_.event_generation[event_],generation_+1,0};
        auto initialize=[&](SoundVoiceSeed* seed,std::string* why){called=true;if(!sound_voice_seed(profile,request.resource,request.program,request.region,request.key,seed,why))return false;initialized=*seed;return true;};
        if(!backend_.acquire_initialized(request.priority,owner,initialize,&acquired,error))return false;
        SoundVoiceAcquireObservation observation{event_,request.region,request.software_slot,request.priority,acquired.acquired,0,-1};u64 stolen_generation=0;
        if(acquired.acquired){if(!called||acquired.lease.identity.event!=owner.event||acquired.lease.identity.slot!=owner.slot||acquired.lease.identity.event_generation!=owner.event_generation||acquired.lease.identity.voice_generation!=owner.voice_generation)return fail("actual initialized owner binding");
            observation.token=acquired.lease.identity.token;
            if(acquired.stolen){const auto& victim=acquired.victim.identity;if(victim.slot>=64||victim.event>=64||!victim.voice_generation||preview_[victim.slot]!=victim.voice_generation||source_.event_generation[victim.event]!=victim.event_generation||victim.token!=observation.token)return fail("actual stolen identity/generation");observation.stolen_voice=s16(victim.slot);stolen_generation=victim.voice_generation;}
            ++generation_;preview_[request.software_slot]=generation_;out->seed=std::move(initialized);out->initialized=true;
        }else if(called)return fail("failed backend acquisition initialized an unread seed");
        out->backend=observation;receipts_.push_back({source_.event_pass,source_.event_generation[event_],observation,stolen_generation});if(error)error->clear();return true;
    }
private:
    SoundCpuBackend& backend_;const SoundVoiceLifecycleState& source_;u16 event_;
    std::vector<SoundVoiceLifecycleAcquisition>& receipts_;u64 generation_;std::array<u64,64> preview_;
};
class EventAllocate final:public SoundVoiceEventAllocator {
public:
    EventAllocate(SoundCpuBackend& backend,std::vector<SoundVoiceLifecycleAcquisition>& receipts):backend_(backend),receipts_(receipts){}
    bool allocate(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const SoundVoiceLifecycleState& state,u16 event,SoundVoiceAllocationPlan* out,u64* final_generation,std::string* error)override{
        if(!final_generation){if(error)*error="native event allocation: generation output required";return false;}
        RegionAcquire provider(backend_,state,event,receipts_);
        if(!sound_voice_allocate_provided(library,profile,state.events,state.voices,event,provider,out,error))return false;
        *final_generation=provider.generation();return true;
    }
private:
    SoundCpuBackend& backend_;std::vector<SoundVoiceLifecycleAcquisition>& receipts_;
};
}
inline bool sound_cpu_event_step(const SoundVoiceLibrary& library,const SoundVoiceProfile& voice,const SoundCpuControlState& input,
        const StartupSoundRing& ring,const SoundVoiceLifecycleEventObservation& observation,SoundCpuEventPlan* out,std::string* error){
    if(out&&(&input==&out->state||&ring==&out->ring)){if(error)*error="native event owner: aliased input/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("native event owner: ")+why;return false;};
    if(!out||!library.ready()||!sound_voice_profile_valid(voice)||!sound_voice_parameter_valid(input.parameters)||!input.backend.matches(input.parameters.lifecycle))return fail("library/profile/joint owner state/output");
    const auto backend=input.backend.snapshot();if(backend.callback!=input.parameters.lifecycle.callback||input.backend.dpop_state().frames!=backend.callback||backend.subframes%5||backend.subframes/5!=input.backend.dpop_state().frames)return fail("unqualified mixed legacy/control frame epoch");
    SoundCpuEventPlan plan;plan.state=input;sound_cpu_event_detail::EventAllocate provider(plan.state.backend,plan.actual_acquisitions);SoundVoiceLifecycleEventPlan events;
    if(!sound_voice_lifecycle_event_provided(library,voice,input.parameters.lifecycle,ring,observation,provider,&events,error))return false;
    plan.state.parameters.lifecycle=std::move(events.state);sound_voice_parameters_detail::sync(plan.state.parameters);plan.ring=std::move(events.ring);plan.initialized=events.initialized;plan.steals=events.steals;plan.unlinked_children=events.unlinked_children;plan.released_events=events.released_events;plan.reentrant_skip=events.reentrant_skip;
    if(!sound_voice_parameter_valid(plan.state.parameters)||!plan.state.backend.matches(plan.state.parameters.lifecycle))return fail("derived software/native/cache owner mismatch");
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
