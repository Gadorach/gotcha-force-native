#pragma once
// EF878 + EE258/EE330/EE66C/EE3B0 + EF390 parameter stores/AX SRC setter.
// Shared scalar/packet consumer of lifecycle plans, NOT backend/PCM execution.
#include "engine/gameplay/sound_voice_lifecycle.hpp"
namespace gf {
struct SoundVoiceRampState {u16 master=0;std::array<u32,2> aux{};bool operator==(const SoundVoiceRampState&)const=default;};
struct SoundVoiceAuxObservation {bool declared=false,present=false;u8 gain_byte=0;};
struct SoundVoiceControlObservation {
    u64 callback=0;bool declared=false;u8 volume=0,master_volume=0,mute=0;u16 aux_flags=0;
    std::array<SoundVoiceAuxObservation,2> aux;
};
struct SoundVoiceBankGainObservation {
    u64 callback=0;u8 bank=0,volume=0;bool declared=false;std::array<u8,128> programs{};
};
struct SoundVoiceParameterCache {
    u64 generation=0;std::array<u16,18> mix{};u16 ve_volume=0,pitch=0;u32 src_q16=0;
    u8 volume=0,pan=0;bool pitch_valid=false;SoundVoicePitch pitched;
};
struct SoundVoiceParameterState {
    SoundVoiceLifecycleState lifecycle;SoundVoiceRampState ramps;
    std::array<SoundVoiceParameterCache,64> voices;
};
struct SoundVoiceParameterPacket {
    SoundVoiceLifecycleIdentity identity;u64 callback=0;
    bool publish=false,start=false,stop_and_free=false,pitch_write=false;
    u8 volume=0,pan=0;u16 ve_volume=0,ve_delta=0;std::array<u16,18> mix{};u32 src_q16=0;
};
struct SoundVoiceParameterEventPlan {SoundVoiceParameterState state;StartupSoundRing ring;unsigned initialized=0,unlinked=0,released=0;bool requires_backend_execution=true;};
struct SoundVoiceParameterPlan {SoundVoiceParameterState state;std::vector<SoundVoiceParameterPacket> packets;bool requires_backend_execution=true;};
namespace sound_voice_parameters_detail {
inline bool ramps_valid(const SoundVoiceRampState& r){return r.master<=0x1fc0&&r.aux[0]<=32767&&r.aux[1]<=32767;}
inline SoundVoiceRampState ramp(const SoundVoiceRampState& r,u8 mute,u16 flags){
    SoundVoiceRampState next=r;next.master=u16(mute?(r.master<677?0:r.master-677):std::min<u32>(0x1fc0,r.master+677));
    for(unsigned i=0;i<2;++i)next.aux[i]=(flags&(1u<<i))?std::min(32767u,r.aux[i]+5461u):(r.aux[i]<5461?0:r.aux[i]-5461u);
    return next;
}
inline u8 volume(u8 base,const std::array<u8,6>& factors){
    // Every divide/multiply has a source single-precision rounding boundary.
    f32 value=f32(f32(base)/127.f);
    for(unsigned i=0;i<factors.size();++i){value=f32(value*f32(factors[i]));if(i+1<factors.size())value=f32(value/127.f);}
    return u8(u32(value)); // bounded byte inputs keep truncation within uint32
}
inline u8 pan(u8 base,u8 event){const s32 center=event==255?64:s32(event);return u8(std::clamp(center+s32(base)-64,0,127));}
inline std::array<u16,18> mix(const std::array<u16,18>& previous,u8 mode,u8 pan,const std::array<std::array<u16,4>,2>& gains){
    auto next=previous;
    if(mode==0)next[0]=next[2]=16384;
    else if(mode==1){next[2]=pan<64?u16(u32(pan)*256u):u16(16384u+u32(pan-64)*16384u/63u);next[0]=u16(32768u-next[2]);}
    // Modes2/3 retain the previous dry pair; no invented DPL2/surround pan.
    for(unsigned bus=0;bus<2;++bus)for(unsigned side=0;side<2;++side){u32 value=next[side*2];for(auto factor:gains[bus])value=u32(value*u32(factor))/32767u;next[4+bus*4+side*2]=u16(value);}
    return next; // source leaves deltas and surround fields untouched
}
inline bool src(const SoundVoiceProfile& profile,const SoundVoicePitch& pitch,u32* out){
    const f32 ratio=f32(pitch.sample_rate/profile.src_rate),scaled=f32(profile.src_scale*ratio);
    if(!std::isfinite(scaled)||scaled<0||f64(scaled)>4294967295.)return false;
    *out=std::min(u32(scaled),0x40000u);return true; // AX setter clamps AFTER conversion
}
inline void sync(SoundVoiceParameterState& state){
    for(unsigned i=0;i<64;++i){const auto generation=state.lifecycle.voice_generation[i];auto& c=state.voices[i];if(!generation)c={};else if(c.generation!=generation){c={};c.generation=generation;const auto& seed=state.lifecycle.voices.voices[i].seed;c.mix=seed.initial_mix;c.ve_volume=seed.initial_ve;c.src_q16=seed.initial_src_q16;}}
}
}
inline bool sound_voice_parameter_valid(const SoundVoiceParameterState& s){
    if(!sound_voice_lifecycle_valid(s.lifecycle)||!sound_voice_parameters_detail::ramps_valid(s.ramps))return false;
    for(unsigned i=0;i<64;++i){const auto& c=s.voices[i];if(c.generation!=s.lifecycle.voice_generation[i])return false;if(c.pitch_valid&&(!std::isfinite(c.pitched.cents)||!std::isfinite(c.pitched.sample_rate)||c.pitched.sample_rate<=0||c.src_q16>0x40000u||c.pitch>16383))return false;}
    return true;
}
inline bool sound_voice_parameter_event_step(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const SoundVoiceParameterState& input,
        const StartupSoundRing& ring,const SoundVoiceLifecycleEventObservation& observation,std::span<const SoundVoiceLifecycleAcquisition> acquisitions,SoundVoiceParameterEventPlan* out,std::string* error){
    if(out&&(&input==&out->state||&ring==&out->ring)){if(error)*error="voice parameter event: aliased input/output";return false;}
    if(out)*out={};
    if(!out||!sound_voice_parameter_valid(input)){if(error)*error="voice parameter event: state/output";return false;}
    SoundVoiceLifecycleEventPlan event;if(!sound_voice_lifecycle_event_step(library,profile,input.lifecycle,ring,observation,acquisitions,&event,error))return false;
    SoundVoiceParameterEventPlan plan;plan.state=input;plan.state.lifecycle=std::move(event.state);plan.ring=std::move(event.ring);plan.initialized=event.initialized;plan.unlinked=event.unlinked_children;plan.released=event.released_events;sound_voice_parameters_detail::sync(plan.state);
    *out=std::move(plan);if(error)error->clear();return true;
}
inline bool sound_voice_parameter_step(const SoundVoiceProfile& profile,const SoundEnvelopeProfile& envelope,const SoundVoiceParameterState& input,
        const SoundVoiceControlObservation& controls,std::span<const SoundVoiceBankGainObservation> banks,
        std::span<const SoundVoiceLifecycleBackendState> backend,SoundVoiceParameterPlan* out,std::string* error){
    if(out && &input==&out->state){if(error)*error="voice parameters: aliased input/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice parameters: ")+why;return false;};
    if(!out||!sound_voice_profile_valid(profile)||!sound_voice_parameter_valid(input)||!controls.declared||controls.callback!=input.lifecycle.callback+1||banks.size()>8)return fail("profile/state/declared callback/explicit bank budget");
    for(unsigned i=0;i<banks.size();++i)for(unsigned j=0;j<i;++j)if(banks[i].bank==banks[j].bank)return fail("duplicate gain bank");
    std::array<u16,2> aux{};for(unsigned i=0;i<2;++i){if(!controls.aux[i].declared)return fail("aux-effect getter observation required even on empty callback");aux[i]=controls.aux[i].present?u16(u32(controls.aux[i].gain_byte)*32767u/255u):0;}
    SoundVoiceLifecycleCallbackPlan lifecycle;if(!sound_voice_lifecycle_callback_step(envelope,input.lifecycle,backend,&lifecycle,error))return false;
    SoundVoiceParameterPlan plan;plan.state=input;plan.state.lifecycle=std::move(lifecycle.state);plan.state.ramps=sound_voice_parameters_detail::ramp(input.ramps,controls.mute,controls.aux_flags);
    std::array<const SoundVoiceBankGainObservation*,64> bank_for_event{};
    for(s16 id=input.lifecycle.events.head;id!=-1;id=input.lifecycle.events.events[unsigned(id)].next12){const auto& event=input.lifecycle.events.events[unsigned(id)];if(event.busy03)continue;
        for(const auto& bank:banks)if(bank.bank==event.bank35)bank_for_event[unsigned(id)]=&bank;
        const auto* bank=bank_for_event[unsigned(id)];if(!bank||!bank->declared||bank->callback!=controls.callback||event.program38>=128)return fail("unbound/stale bank/program gain table");
    }
    for(const auto& update:lifecycle.updates){const unsigned slot=update.identity.slot,event=update.identity.event;const auto& source=input.lifecycle.voices.voices[slot];const auto& seed=source.seed;const auto& e=input.lifecycle.events.events[event];auto& c=plan.state.voices[slot];SoundVoiceParameterPacket packet;packet.identity=update.identity;packet.callback=controls.callback;packet.publish=update.envelope.publish_parameters;packet.start=update.envelope.start_backend;packet.stop_and_free=update.envelope.stop_and_free_backend;
        if(packet.publish){const auto& bank=*bank_for_event[event];const std::array<u8,6> factors={bank.programs[e.program38],bank.volume,e.volume28,controls.volume,controls.master_volume,u8(plan.state.ramps.master>>6&127u)};
            c.volume=sound_voice_parameters_detail::volume(seed.base_volume,factors);c.pan=sound_voice_parameters_detail::pan(seed.base_pan,e.pan29);c.ve_volume=u16(u32(update.envelope.state.level>>12)*c.volume/127u);
            std::array<std::array<u16,4>,2> gains={std::array<u16,4>{seed.channel_gain[0],e.limit2c,aux[0],u16(plan.state.ramps.aux[0])},std::array<u16,4>{seed.channel_gain[1],e.limit2e,aux[1],u16(plan.state.ramps.aux[1])}};
            c.mix=sound_voice_parameters_detail::mix(c.mix,input.lifecycle.output_mode,c.pan,gains);
            if(e.dirty30&4u){if(!sound_voice_pitch(profile,seed,e.pitch2a,&c.pitched,error))return false;if(!sound_voice_parameters_detail::src(profile,c.pitched,&c.src_q16))return fail("unqualified float-to-unsigned backend SRC range");c.pitch=e.pitch2a;c.pitch_valid=true;packet.pitch_write=true;}
            packet.volume=c.volume;packet.pan=c.pan;packet.ve_volume=c.ve_volume;packet.mix=c.mix;packet.src_q16=c.src_q16;
        }
        plan.packets.push_back(packet);
    }
    // All required source parameter calculations have now succeeded. Backend
    // application/stop/start/PCM is STILL required before committing this plan.
    for(s16 id=plan.state.lifecycle.events.head;id!=-1;id=plan.state.lifecycle.events.events[unsigned(id)].next12)if(!plan.state.lifecycle.events.events[unsigned(id)].busy03){plan.state.lifecycle.events.events[unsigned(id)].dirty30=0;plan.state.lifecycle.dirty32[unsigned(id)]=0;}
    if(!sound_voice_parameter_valid(plan.state))return fail("derived parameter state");
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
