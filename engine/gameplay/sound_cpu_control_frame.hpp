#pragma once
// Joint native source/CPU packet transaction. Observe/plan from the prior frame,
// render this frame using previous writes, then apply current callback writes.
// This preserves SyncVoices-before-EF390 command visibility without inventing a
// game/EDA40 clock. Automatic region acquisition and queue admission stay open.
#include "engine/gameplay/sound_cpu_backend.hpp"
namespace gf {
struct SoundCpuControlState {SoundVoiceParameterState parameters;SoundCpuBackend backend;};
struct SoundCpuControlPlan {SoundCpuControlState state;SoundCpuDropFrame pcm;unsigned actual_backend_reads=0,packets_applied=0;bool requires_bus_queue_consumer=true;};
inline bool sound_cpu_control_frame(const SoundVoiceProfile& voice,const SoundEnvelopeProfile& envelope,const SoundCpuControlState& input,
        const SoundVoiceControlObservation& controls,std::span<const SoundVoiceBankGainObservation> banks,SoundCpuControlPlan* out,std::string* error){
    if(out && &input==&out->state){if(error)*error="CPU control frame: aliased state/output";return false;}
    if(out)*out={};
    if(!out||!input.backend.matches(input.parameters.lifecycle)||!sound_voice_parameter_valid(input.parameters)||input.backend.dpop_state().frames!=input.parameters.lifecycle.callback){if(error)*error="CPU control frame: joint owner state/frame epoch/output";return false;}
    std::vector<SoundVoiceLifecycleBackendState> observed;if(!input.backend.observe(input.backend.scope(),input.parameters.lifecycle,&observed,error))return false;
    SoundVoiceParameterPlan parameters;if(!sound_voice_parameter_step(voice,envelope,input.parameters,controls,banks,observed,&parameters,error))return false;
    SoundCpuControlPlan plan;plan.state=input;plan.actual_backend_reads=unsigned(observed.size());plan.packets_applied=unsigned(parameters.packets.size());
    if(!plan.state.backend.render_drop_frame(plan.state.backend.scope(),&plan.pcm,error))return false;
    if(!plan.state.backend.apply_callback(plan.state.backend.scope(),parameters.state.lifecycle.callback,parameters.packets,error))return false;
    plan.state.parameters=std::move(parameters.state);
    if(!plan.state.backend.matches(plan.state.parameters.lifecycle)){if(error)*error="CPU control frame: derived owner mismatch";return false;}
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
