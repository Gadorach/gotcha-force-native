#pragma once
// Shared EDA40 event phases + EF390 envelope/ownership join. Pure candidates:
// acquisitions/backend states are declared receipts, not executed audio calls.
// Full gain/pan/pitch/mix consumer and manager completion remain REQUIRED.
#include <limits>
#include "engine/gameplay/sound_voice_allocate.hpp"
#include "engine/gameplay/sound_voice_envelope.hpp"
namespace gf {
struct SoundVoiceLifecycleState {
    StartupSoundEventPool events=startup_sound_event_pool_fresh();
    SoundSoftwareVoicePool voices=sound_software_voice_pool_fresh();
    std::array<SoundEnvelopeState,64> envelopes{};
    std::array<u64,64> event_generation{},voice_generation{};
    std::array<u16,64> dirty32{};
    u64 generation=0,event_pass=0,callback=0;
    u16 pending_flags=0;u8 output_mode=0;bool event_busy=false;
};
struct SoundVoiceLifecycleIdentity {
    u16 event=0,slot=0;u64 event_generation=0,voice_generation=0;u32 token=0;
    bool operator==(const SoundVoiceLifecycleIdentity&)const=default;
};
struct SoundVoiceLifecycleAcquisition {
    u64 event_pass=0,event_generation=0;SoundVoiceAcquireObservation backend;
    u64 stolen_generation=0;
};
struct SoundVoiceLifecycleBackendState {
    u64 callback=0;SoundVoiceLifecycleIdentity identity;u16 state=0;
};
struct SoundVoiceLifecycleEventObservation {bool output_mode_declared=false;u8 output_mode=0;};
struct SoundVoiceLifecycleEventPlan {
    SoundVoiceLifecycleState state;StartupSoundRing ring;
    unsigned dispatched=0,initialized=0,unlinked_children=0,released_events=0,steals=0;
    bool reentrant_skip=false,requires_voice_parameter_consumer=true;
};
class SoundVoiceEventAllocator {
public:
    virtual ~SoundVoiceEventAllocator()=default;
    virtual bool allocate(const SoundVoiceLibrary&,const SoundVoiceProfile&,const SoundVoiceLifecycleState&,u16,SoundVoiceAllocationPlan*,u64* final_generation,std::string*)=0;
};
struct SoundVoiceLifecycleUpdate {
    SoundVoiceLifecycleIdentity identity;SoundEnvelopePlan envelope;
    u16 event_dirty30=0,event_dirty32=0;
};
struct SoundVoiceLifecycleCallbackPlan {
    SoundVoiceLifecycleState state;std::vector<SoundVoiceLifecycleUpdate> updates;
    unsigned skipped_busy_events=0,backend_reads=0;
    bool requires_voice_parameter_consumer=true;
};
inline SoundVoiceLifecycleIdentity sound_voice_lifecycle_identity(const SoundVoiceLifecycleState& s,u16 slot){
    if(slot>=64||!s.voices.voices[slot].allocated)return {};
    const auto& v=s.voices.voices[slot];if(v.parent<0||v.parent>=64)return {};
    return {u16(v.parent),slot,s.event_generation[unsigned(v.parent)],s.voice_generation[slot],v.backend_token};
}
inline bool sound_voice_lifecycle_valid(const SoundVoiceLifecycleState& s){
    if(!sound_software_voice_pool_valid(s.events,s.voices)||s.output_mode>3)return false;
    std::array<u64,128> generations{};unsigned n=0;
    auto unique=[&](u64 g){if(!g||g>s.generation)return false;for(unsigned j=0;j<n;++j)if(generations[j]==g)return false;generations[n++]=g;return true;};
    for(unsigned i=0;i<64;++i){
        if(s.events.events[i].allocated){const auto& e=s.events.events[i];if(!unique(s.event_generation[i])||e.phase0c>3||(e.phase0c==0&&e.child14!=-1))return false;}
        else if(s.event_generation[i])return false;
        const auto& v=s.voices.voices[i];const auto& e=s.envelopes[i];
        if(v.allocated){if(!unique(s.voice_generation[i])||e.phase>6||e.done!=v.done24||(e.done&&e.phase!=6)||(e.phase>0&&e.phase<6&&e.level>0x7ffffffu))return false;}
        else if(s.voice_generation[i])return false;
    }
    return true;
}
namespace sound_voice_lifecycle_detail {
inline bool generation(SoundVoiceLifecycleState& s,u64* out){if(s.generation==std::numeric_limits<u64>::max())return false;*out=++s.generation;return true;}
inline void release_event(SoundVoiceLifecycleState& s,u16 id){
    auto& p=s.events;auto& e=p.events[id];
    if(e.previous0e==-1)p.head=e.next12;else p.events[unsigned(e.previous0e)].next12=e.next12;
    if(e.next12==-1)p.tail=e.previous0e;else p.events[unsigned(e.next12)].previous0e=e.previous0e;
    e.next12=e.previous0e=-1;e.allocated=false;p.free[p.free_count++]=u8(id);s.event_generation[id]=0;
}
}
namespace sound_voice_lifecycle_detail {
inline bool event_step(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,
        const SoundVoiceLifecycleState& input,const StartupSoundRing& ring,const SoundVoiceLifecycleEventObservation& observation,
        std::span<const SoundVoiceLifecycleAcquisition> acquisitions,SoundVoiceEventAllocator* provider,SoundVoiceLifecycleEventPlan* out,std::string* error){
    if(out&&(&input==&out->state||&ring==&out->ring)){if(error)*error="voice lifecycle events: aliased input/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice lifecycle events: ")+why;return false;};
    if(!out||!sound_voice_lifecycle_valid(input)||acquisitions.size()>64*255)return fail("state/receipt budget");
    SoundVoiceLifecycleEventPlan plan;plan.state=input;plan.ring=ring;
    if(input.event_busy){if(!acquisitions.empty())return fail("unexpected receipts on reentrant skip");plan.reentrant_skip=true;*out=std::move(plan);if(error)error->clear();return true;}
    if(!observation.output_mode_declared||observation.output_mode>3||input.event_pass==std::numeric_limits<u64>::max())return fail("declared output mode/native pass budget");
    auto& s=plan.state;++s.event_pass;s.event_busy=true;s.output_mode=observation.output_mode;
    StartupSoundDispatchPlan prefix;if(!startup_sound_dispatch_prefix(ring,s.events,&prefix,error))return false;
    s.events=std::move(prefix.pool);plan.ring=std::move(prefix.ring);plan.dispatched=prefix.count;
    for(unsigned i=0;i<prefix.count;++i)if(prefix.outcomes[i].allocated_slot!=-1){const auto slot=unsigned(prefix.outcomes[i].allocated_slot);if(s.event_generation[slot]||!sound_voice_lifecycle_detail::generation(s,&s.event_generation[slot]))return fail("event generation binding/exhaustion");}
    unsigned consumed=0;
    for(s16 id=s.events.head;id!=-1;){const u16 event=u16(id);auto& e=s.events.events[event];e.busy03=1;s.dirty32[event]=s.pending_flags;
        const u16 phase=e.phase0c;bool cleanup=false;
        if(phase==0){
            if(e.requested08==0x10||e.requested08==0x20){e.flag01=1;e.phase0c=3;e.current04=e.requested08=0x10;}
            else{
                SoundVoiceAllocationPlan allocated;u64 preview_generation=s.generation;
                if(provider){if(!provider->allocate(library,profile,s,event,&allocated,&preview_generation,error))return false;}
                else{std::vector<SoundVoiceAcquireObservation> receipts;auto preview_ids=s.voice_generation;
                while(consumed<acquisitions.size()&&acquisitions[consumed].backend.event==event){const auto& a=acquisitions[consumed++];const auto& backend=a.backend;
                    if(a.event_pass!=s.event_pass||a.event_generation!=s.event_generation[event])return fail("acquisition pass/event generation");
                    if(backend.stolen_voice!=-1){if(backend.stolen_voice<0||backend.stolen_voice>=64||!a.stolen_generation||a.stolen_generation!=preview_ids[unsigned(backend.stolen_voice)])return fail("stale stolen-voice generation");}
                    else if(a.stolen_generation)return fail("unexpected stolen generation");
                    if(backend.acquired){if(backend.software_slot>=64||preview_generation==std::numeric_limits<u64>::max())return fail("preview voice generation/slot budget");preview_ids[backend.software_slot]=++preview_generation;}
                    receipts.push_back(backend);
                }
                if(!sound_voice_allocate(library,profile,s.events,s.voices,event,receipts,&allocated,error))return false;}
                // Copy fields, not s itself: e remains bound to this array element.
                s.events=std::move(allocated.events);s.voices=std::move(allocated.voices);plan.initialized+=allocated.initialized;plan.steals+=allocated.steals;
                for(const auto& r:allocated.regions)if(r.software_slot!=-1&&!r.backend_failed){const auto slot=unsigned(r.software_slot);if(s.voice_generation[slot]||!sound_voice_lifecycle_detail::generation(s,&s.voice_generation[slot]))return fail("voice generation binding/exhaustion");s.envelopes[slot]={};}
                if(s.generation!=preview_generation)return fail("generation/receipt initialization count");
                e.phase0c=e.flags3a==0?2:1;
            }
        }else{
            if(phase==1){e.flags3a=u16(e.flags3a-4u);if(std::bit_cast<s16>(e.flags3a)<1){e.flag01=1;e.phase0c=3;e.current04=e.requested08=0x20;}}
            if(phase==1||phase==2){if(e.requested08==0x10||e.requested08==0x20){e.flag01=1;e.phase0c=3;e.current04=e.requested08;}cleanup=true;}
            else if(phase==3)cleanup=true;
        }
        if(cleanup){e.wait5a=0;for(s16 child=e.child14;child!=-1;){const auto slot=u16(child);const auto next=s.voices.voices[slot].next;
                if(s.voices.voices[slot].done24){sound_voice_allocate_detail::remove(s.events,s.voices,event,slot);s.voice_generation[slot]=0;s.envelopes[slot]={};++plan.unlinked_children;}
                else e.wait5a=u16(e.wait5a+1u);
                child=next;
            }
            if(e.child14==-1)e.flag02=1;
        }
        e.busy03=0;id=e.next12;
        if(e.flag02){if(e.child14!=-1||s.voices.event_tails[event]!=-1)return fail("event release with retained children");sound_voice_lifecycle_detail::release_event(s,event);++plan.released_events;}
    }
    if(consumed!=acquisitions.size())return fail("unconsumed/out-of-order acquisition receipts");
    s.pending_flags=0;s.event_busy=false;
    if(!sound_voice_lifecycle_valid(s))return fail("derived state topology/generation");
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace sound_voice_lifecycle_detail
inline bool sound_voice_lifecycle_event_step(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const SoundVoiceLifecycleState& input,const StartupSoundRing& ring,const SoundVoiceLifecycleEventObservation& observation,std::span<const SoundVoiceLifecycleAcquisition> acquisitions,SoundVoiceLifecycleEventPlan* out,std::string* error){return sound_voice_lifecycle_detail::event_step(library,profile,input,ring,observation,acquisitions,nullptr,out,error);}
inline bool sound_voice_lifecycle_event_provided(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const SoundVoiceLifecycleState& input,const StartupSoundRing& ring,const SoundVoiceLifecycleEventObservation& observation,SoundVoiceEventAllocator& provider,SoundVoiceLifecycleEventPlan* out,std::string* error){return sound_voice_lifecycle_detail::event_step(library,profile,input,ring,observation,{},&provider,out,error);}
inline bool sound_voice_lifecycle_callback_step(const SoundEnvelopeProfile& profile,const SoundVoiceLifecycleState& input,
        std::span<const SoundVoiceLifecycleBackendState> observations,SoundVoiceLifecycleCallbackPlan* out,std::string* error){
    if(out && &input==&out->state){if(error)*error="voice lifecycle callback: aliased input/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice lifecycle callback: ")+why;return false;};
    if(!out||!profile.ready()||!sound_voice_lifecycle_valid(input)||observations.size()>64||input.callback==std::numeric_limits<u64>::max())return fail("profile/state/observation/native callback budget");
    SoundVoiceLifecycleCallbackPlan plan;plan.state=input;auto& s=plan.state;++s.callback;unsigned consumed=0;
    for(s16 id=s.events.head;id!=-1;id=s.events.events[unsigned(id)].next12){const auto& e=s.events.events[unsigned(id)];if(e.busy03){++plan.skipped_busy_events;continue;}
        for(s16 child=e.child14;child!=-1;child=s.voices.voices[unsigned(child)].next){const auto slot=u16(child);auto& v=s.voices.voices[slot];SoundVoiceLifecycleUpdate update;update.identity=sound_voice_lifecycle_identity(s,slot);update.event_dirty30=e.dirty30;update.event_dirty32=s.dirty32[unsigned(id)];
            SoundEnvelopeObservation o;o.event_stop=e.flag01;o.requested=e.requested08;o.stolen=v.stolen25;
            const u8 phase=s.envelopes[slot].phase;const bool reads=!o.stolen&&!e.flag01&&phase<5;
            if(reads){if(consumed>=observations.size())return fail("missing backend-state receipt");const auto& r=observations[consumed++];if(r.callback!=s.callback||r.identity!=update.identity||!sound_voice_backend_owned(v))return fail("stale/foreign backend-state receipt");o.backend_state_declared=true;o.backend_state=r.state;}
            if(!sound_voice_envelope_step(profile,v.seed.envelope_word,s.envelopes[slot],o,&update.envelope,error))return false;
            if((update.envelope.publish_parameters||update.envelope.stop_and_free_backend)&&!sound_voice_backend_owned(v))return fail("attempted parameter/start/stop/free on nonowned token");
            s.envelopes[slot]=update.envelope.state;v.done24=s.envelopes[slot].done;plan.backend_reads+=update.envelope.backend_reads;plan.updates.push_back(update);
        }
        // EF390 clears these AFTER gain/pan/pitch writes. They are deliberately
        // retained here while that required parameter consumer is still absent.
    }
    if(consumed!=observations.size())return fail("unconsumed/out-of-order backend-state receipts");
    if(!sound_voice_lifecycle_valid(s))return fail("derived topology/envelope ownership");
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
