#pragma once
// EDC60 +EEF30/EEFB0/EF070 +EDE48: source 64 software voices and observed AX
// acquisition outcomes. Pure plans do not invent a successful backend, start
// playback, advance envelopes or fulfill EDA40/actor sound requirements.
#include "engine/gameplay/sound_voice_seed.hpp"
#include "engine/gameplay/startup_sound_dispatch.hpp"
namespace gf {
struct SoundSoftwareVoice {
    bool allocated=false,done24=false,stolen25=false,seed_valid=false;u16 slot=0;s16 parent=-1,previous=-1,next=-1;u32 backend_token=0;SoundVoiceSeed seed;
};
struct SoundSoftwareVoicePool {
    std::array<SoundSoftwareVoice,64> voices;std::array<u8,64> free{};u32 free_count=0;
    std::array<s16,64> event_tails=[](){std::array<s16,64> a{};a.fill(-1);return a;}();bool stolen_notification=false;
};
inline SoundSoftwareVoicePool sound_software_voice_pool_fresh(){SoundSoftwareVoicePool p;p.free_count=64;for(unsigned i=0;i<64;++i){p.free[i]=u8(63-i);p.voices[i].slot=u16(i);}return p;}
// EF390 releases the backend before EDA40 unlinks the done software child.
// A completed/stolen child can therefore retain a historical token but no
// longer owns it; reuse need not invent another EDE48 steal callback.
inline bool sound_voice_backend_owned(const SoundSoftwareVoice& voice){return voice.allocated&&!voice.done24&&!voice.stolen25&&voice.backend_token!=0;}
struct SoundVoiceAcquireObservation {u16 event=0,region=0;u8 software_slot=0,priority=0;bool acquired=false;u32 token=0;s16 stolen_voice=-1;};
struct SoundVoiceAcquireRequest {std::shared_ptr<const SoundVoiceBankResource> resource;u16 event=0,program=0,region=0;u8 key=0,software_slot=0,priority=0;};
struct SoundVoiceAcquireReply {SoundVoiceAcquireObservation backend;SoundVoiceSeed seed;bool initialized=false;};
class SoundVoiceAcquireProvider {
public:
    virtual ~SoundVoiceAcquireProvider()=default;
    // Called ONLY by the common region traversal at an actual backend attempt.
    // Return an initialized seed only on real acquisition success. The provider
    // must be staged by its owner; late native guards are not source atomicity.
    virtual bool acquire(const SoundVoiceProfile&,const SoundVoiceAcquireRequest&,SoundVoiceAcquireReply*,std::string*)=0;
};
struct SoundVoiceRegionOutcome {u16 region=0,sample=0,dsp=0;u8 priority=0;s16 software_slot=-1;bool key_matched=false,invalid_sample=false,invalid_dsp=false,pool_full=false,backend_failed=false;};
struct SoundVoiceAllocationPlan {
    StartupSoundEventPool events;SoundSoftwareVoicePool voices;std::vector<SoundVoiceRegionOutcome> regions;unsigned backend_attempts=0,initialized=0,steals=0;bool invalid_program=false,requires_voice_update=false;
};
inline bool sound_software_voice_pool_valid(const StartupSoundEventPool& events,const SoundSoftwareVoicePool& voices){
    if(!startup_sound_dispatch_detail::pool_valid(events)||voices.free_count>64)return false;
    std::array<bool,64> seen{};unsigned allocated=0;
    for(unsigned event=0;event<64;++event){const auto& e=events.events[event];s16 id=e.child14,previous=-1;unsigned length=0;if(!e.allocated&&(id!=-1||voices.event_tails[event]!=-1))return false;
        while(id!=-1){if(id<0||id>=64||seen[unsigned(id)]||++length>64)return false;const auto& v=voices.voices[unsigned(id)];if(!v.allocated||v.slot!=u16(id)||v.parent!=s16(event)||v.previous!=previous||!v.seed_valid||!v.backend_token||!v.seed.resource||v.seed.resource->id!=e.bank35||v.seed.program!=e.program38)return false;seen[unsigned(id)]=true;++allocated;previous=id;id=v.next;}
        if(previous!=voices.event_tails[event])return false;
    }
    if(allocated+voices.free_count!=64)return false;
    for(unsigned i=0;i<voices.free_count;++i){const unsigned id=voices.free[i];if(id>=64||seen[id])return false;const auto& v=voices.voices[id];if(v.allocated||v.slot!=id||v.previous!=-1||v.next!=-1)return false;seen[id]=true;}
    for(unsigned i=0;i<64;++i)if(sound_voice_backend_owned(voices.voices[i]))for(unsigned j=0;j<i;++j)if(sound_voice_backend_owned(voices.voices[j])&&voices.voices[i].backend_token==voices.voices[j].backend_token)return false;
    return true;
}
namespace sound_voice_allocate_detail {
inline void append(StartupSoundEventPool& events,SoundSoftwareVoicePool& pool,u16 event,u16 slot){auto& e=events.events[event];auto& v=pool.voices[slot];v.allocated=true;v.parent=s16(event);v.done24=v.stolen25=false;v.seed_valid=false;v.previous=pool.event_tails[event];v.next=-1;if(v.previous==-1)e.child14=s16(slot);else pool.voices[unsigned(v.previous)].next=s16(slot);pool.event_tails[event]=s16(slot);}
inline void remove(StartupSoundEventPool& events,SoundSoftwareVoicePool& pool,u16 event,u16 slot){auto& e=events.events[event];auto& v=pool.voices[slot];if(v.previous==-1)e.child14=v.next;else pool.voices[unsigned(v.previous)].next=v.next;if(v.next==-1)pool.event_tails[event]=v.previous;else pool.voices[unsigned(v.next)].previous=v.previous;v.previous=v.next=-1;v.allocated=false;v.seed_valid=false;pool.free[pool.free_count++]=u8(slot);}
}
namespace sound_voice_allocate_detail {
inline bool plan(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const StartupSoundEventPool& events,const SoundSoftwareVoicePool& voices,u16 event,std::span<const SoundVoiceAcquireObservation> observations,SoundVoiceAcquireProvider* provider,SoundVoiceAllocationPlan* out,std::string* error){
    if(out&&(&events==&out->events||&voices==&out->voices)){if(error)*error="voice allocation: input/output alias refused without mutating input pools";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice allocation: ")+why;return false;};
    if(!out||!library.ready()||!sound_voice_profile_valid(profile)||event>=64||!events.events[event].allocated||events.events[event].phase0c!=0||observations.size()>255||!sound_software_voice_pool_valid(events,voices))return fail("library/profile/event phase/pool topology");
    auto resource=library.find(events.events[event].bank35);if(!resource)return fail("unbound event bank, no bank0 substitution");SoundVoiceAllocationPlan plan;plan.events=events;plan.voices=voices;auto& e=plan.events.events[event];plan.requires_voice_update=true;const auto program=e.program38;
    if(program>=resource->bank.programs.size()||!resource->bank.programs[program].present){if(!observations.empty())return fail("unexpected backend observations for invalid program");e.flag02=1;plan.invalid_program=true;*out=std::move(plan);if(error)error->clear();return true;}
    const auto& p=resource->bank.programs[program];if(p.regions.size()>255||p.raw[0]!=p.regions.size())return fail("program region extent");
    for(unsigned i=0;i<p.regions.size();++i){const auto& r=p.regions[i];SoundVoiceRegionOutcome result;result.region=u16(i);result.sample=r.sample;result.priority=r.raw[0];if(r.raw[2]>r.raw[3]||r.sample!=load_be16(r.raw.data()+12))return fail("region metadata binding");result.key_matched=r.raw[2]<=e.key39&&e.key39<=r.raw[3];
        if(result.key_matched){if(r.sample>=resource->bank.samples.size())result.invalid_sample=true;else{const auto& sample=resource->bank.samples[r.sample];if(sample.dsp!=load_be16(sample.raw.data()+10))return fail("sample metadata binding");result.dsp=sample.dsp;if(sample.dsp>=resource->bank.dsp.size())result.invalid_dsp=true;else if(!plan.voices.free_count)result.pool_full=true;else{
                    const auto slot=plan.voices.free[--plan.voices.free_count];result.software_slot=s16(slot);sound_voice_allocate_detail::append(plan.events,plan.voices,event,slot);auto& voice=plan.voices.voices[slot];
                    SoundVoiceAcquireObservation acquired;SoundVoiceSeed initialized;bool has_seed=false;
                    if(provider){SoundVoiceAcquireReply reply;if(!provider->acquire(profile,{resource,event,program,u16(i),e.key39,slot,r.raw[0]},&reply,error))return false;acquired=reply.backend;initialized=std::move(reply.seed);has_seed=reply.initialized;if(has_seed!=acquired.acquired)return fail("provider seed/acquisition outcome mismatch");++plan.backend_attempts;}
                    else{if(plan.backend_attempts>=observations.size())return fail("missing actual backend acquisition observation");acquired=observations[plan.backend_attempts++];}
                    if(acquired.event!=event||acquired.region!=i||acquired.software_slot!=slot||acquired.priority!=r.raw[0]||acquired.stolen_voice<-1||acquired.stolen_voice>=64)return fail("backend observation ownership/order/priority");
                    if(!acquired.acquired){if(acquired.token||acquired.stolen_voice!=-1)return fail("failed acquisition cannot own or steal a backend");voice.backend_token=0;sound_voice_allocate_detail::remove(plan.events,plan.voices,event,slot);result.backend_failed=true;}
                    else{if(!acquired.token)return fail("successful acquisition requires a backend token");if(acquired.stolen_voice!=-1){auto& old=plan.voices.voices[unsigned(acquired.stolen_voice)];if(!sound_voice_backend_owned(old)||!old.seed_valid||old.backend_token!=acquired.token)return fail("stolen voice callback binding");old.stolen25=true;plan.voices.stolen_notification=true;++plan.steals;}
                        for(unsigned j=0;j<64;++j)if(j!=slot&&sound_voice_backend_owned(plan.voices.voices[j])&&plan.voices.voices[j].backend_token==acquired.token)return fail("unreported backend token reuse");
                        voice.backend_token=acquired.token;
                        if(has_seed){if(initialized.resource!=resource||initialized.program!=program||initialized.region!=i||initialized.sample!=r.sample||initialized.dsp!=sample.dsp||initialized.priority!=r.raw[0])return fail("provider initialized seed identity");voice.seed=std::move(initialized);}
                        else if(!sound_voice_seed(profile,resource,program,u16(i),e.key39,&voice.seed,error))return false;
                        voice.seed_valid=true;e.count58=u16(e.count58+1u);e.wait5a=u16(e.wait5a+1u);++plan.initialized;}
                }}}
        plan.regions.push_back(result);
    }
    if(!provider&&plan.backend_attempts!=observations.size())return fail("unconsumed backend observations");
    if(!sound_software_voice_pool_valid(plan.events,plan.voices))return fail("derived pool topology");
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace sound_voice_allocate_detail
inline bool sound_voice_allocate(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const StartupSoundEventPool& events,const SoundSoftwareVoicePool& voices,u16 event,std::span<const SoundVoiceAcquireObservation> observations,SoundVoiceAllocationPlan* out,std::string* error){return sound_voice_allocate_detail::plan(library,profile,events,voices,event,observations,nullptr,out,error);}
inline bool sound_voice_allocate_provided(const SoundVoiceLibrary& library,const SoundVoiceProfile& profile,const StartupSoundEventPool& events,const SoundSoftwareVoicePool& voices,u16 event,SoundVoiceAcquireProvider& provider,SoundVoiceAllocationPlan* out,std::string* error){return sound_voice_allocate_detail::plan(library,profile,events,voices,event,{},&provider,out,error);}
} // namespace gf
