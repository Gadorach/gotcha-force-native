#pragma once
// Optional event pass + audio control frame in ONE native candidate. Event due
// is explicit caller policy; this API does not invent EDA40/simulation cadence.
// The ring and all state remain uncommitted until the shared queue consumer.
#include "engine/gameplay/sound_cpu_events.hpp"
namespace gf {
struct SoundCpuTickObservation {bool events_due=false;SoundVoiceLifecycleEventObservation events;};
struct SoundCpuTickPlan {
    SoundCpuControlState state;StartupSoundRing ring;SoundCpuDropFrame pcm;
    std::vector<SoundVoiceLifecycleAcquisition> actual_acquisitions;
    unsigned initialized=0,unlinked_children=0,released_events=0;
    bool advanced_events=false,requires_bus_queue_consumer=true;
};
inline bool sound_cpu_tick(const SoundVoiceLibrary& library,const SoundVoiceProfile& voice,const SoundEnvelopeProfile& envelope,
        const SoundCpuControlState& input,const StartupSoundRing& ring,const SoundCpuTickObservation& event_observation,
        const SoundVoiceControlObservation& controls,std::span<const SoundVoiceBankGainObservation> banks,SoundCpuTickPlan* out,std::string* error){
    if(out&&(&input==&out->state||&ring==&out->ring)){if(error)*error="CPU tick: aliased state/ring/output";return false;}
    if(out)*out={};
    if(!out){if(error)*error="CPU tick: output required";return false;}
    SoundCpuTickPlan plan;plan.state=input;plan.ring=ring;plan.advanced_events=event_observation.events_due;
    if(event_observation.events_due){SoundCpuEventPlan event;if(!sound_cpu_event_step(library,voice,plan.state,ring,event_observation.events,&event,error))return false;plan.state=std::move(event.state);plan.ring=std::move(event.ring);plan.actual_acquisitions=std::move(event.actual_acquisitions);plan.initialized=event.initialized;plan.unlinked_children=event.unlinked_children;plan.released_events=event.released_events;}
    SoundCpuControlPlan control;if(!sound_cpu_control_frame(voice,envelope,plan.state,controls,banks,&control,error))return false;
    plan.state=std::move(control.state);plan.pcm=std::move(control.pcm);*out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
