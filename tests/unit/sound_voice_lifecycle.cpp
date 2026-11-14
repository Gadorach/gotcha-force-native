#include <cstdio>
#include <cstdlib>
#include "tests/unit/sound_voice_lifecycle_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"voice lifecycle: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace sound_voice_lifecycle_fixture;std::string error;
    sound_voice_fixture::Bank bank;SoundVoiceLibrary library;const std::array<SoundVoiceBankInput,1> inputs={bank.input()};check(library.load(inputs,&error),"owned sound resource");
    SoundVoiceProfile profile;auto p=sound_voice_fixture::profile();check(sound_voice_profile_load({p.data(),p.size()},&profile,&error),"voice scalar profile");SoundEnvelopeProfile envelope;auto q=sound_envelope_fixture::profile();check(envelope.load({q.data(),q.size()},&error),"owned analytic envelope profile");
    SoundVoiceLifecycleState initial;check(sound_voice_lifecycle_valid(initial),"fresh native owner state");const StartupSoundRing empty;auto commands=ring();const SoundVoiceLifecycleEventObservation output{true,1};std::array<SoundVoiceLifecycleAcquisition,2> acquisitions={acquire(0,0,0,100),acquire(0,1,1,101)};SoundVoiceLifecycleEventPlan e;
    check(sound_voice_lifecycle_event_step(library,profile,initial,commands,output,acquisitions,&e,&error)&&e.dispatched==1&&e.initialized==2,"prefix and phase0 consume explicit acquisition receipts");
    auto state=e.state;check(state.events.events[0].phase0c==2&&state.event_generation[0]==1&&state.voice_generation[0]==2&&state.voice_generation[1]==3&&state.voices.free_count==62&&!state.events.events[0].busy03,"generation bindings and zero countdown phase2");
    check(state.events.events[0].count58==2&&state.events.events[0].wait5a==2&&state.event_pass==1&&e.ring.read==e.ring.write&&e.requires_voice_parameter_consumer,"source counters and unfulfilled parameter-consumer guard");
    SoundVoiceLifecycleCallbackPlan c;auto obs=observations(state,0);check(sound_voice_lifecycle_callback_step(envelope,state,obs,&c,&error)&&c.updates.size()==2&&c.updates[0].envelope.start_backend&&c.updates[1].envelope.start_backend,"initial backend0 starts both source layers");
    state=c.state;check(state.envelopes[0].phase==1&&state.callback==1&&state.events.events[0].dirty30==0xffff&&c.requires_voice_parameter_consumer,"envelope joined but dirty state retained until required gain/pan/pitch consumer");
    auto stale=obs;check(!sound_voice_lifecycle_callback_step(envelope,state,stale,&c,&error)&&c.updates.empty(),"callback receipt cannot be replayed on next callback");
    obs=observations(state,0);check(sound_voice_lifecycle_callback_step(envelope,state,obs,&c,&error)&&c.state.envelopes[0].phase==4&&!c.updates[0].envelope.stop_and_free_backend,"later backend0 schedules release, does not immediately free");
    state=c.state;obs=observations(state,0);check(sound_voice_lifecycle_callback_step(envelope,state,obs,&c,&error)&&c.updates[0].envelope.stop_and_free_backend&&c.state.voices.voices[0].done24&&c.state.voices.free_count==62,"release marks done but leaves software children linked");
    state=c.state;const auto old_identity=sound_voice_lifecycle_identity(state,0);check(!sound_voice_backend_owned(state.voices.voices[0]),"done child no longer owns backend token");
    check(sound_voice_lifecycle_callback_step(envelope,state,{},&c,&error)&&!c.updates[0].envelope.stop_and_free_backend,"terminal child never double-frees backend");
    check(sound_voice_lifecycle_event_step(library,profile,state,empty,output,{},&e,&error)&&e.unlinked_children==2&&e.released_events==1&&e.state.events.free_count==64&&e.state.voices.free_count==64,"next EDA40 cleans both children then releases event");
    state=e.state;check(!state.voice_generation[0]&&!state.event_generation[0]&&sound_voice_lifecycle_valid(state),"retired generations cleared with valid free topology");
    auto reuse=ring();std::array<SoundVoiceLifecycleAcquisition,2> reused={acquire(0,0,1,100,4,3),acquire(0,1,0,101,4,3)};
    check(sound_voice_lifecycle_event_step(library,profile,state,reuse,output,reused,&e,&error)&&e.state.event_generation[0]==4&&e.state.voice_generation[0]==6,"event and child LIFO reuse gets new generations even for reused backend numbers");
    state=e.state;obs=observations(state);obs[0].identity=old_identity;obs[0].callback=state.callback+1;check(!sound_voice_lifecycle_callback_step(envelope,state,obs,&c,&error),"same token cannot disguise retired event/voice generation");
    auto busy=state;busy.events.events[0].busy03=1;check(sound_voice_lifecycle_callback_step(envelope,busy,{},&c,&error)&&c.updates.empty()&&c.skipped_busy_events==1&&c.state.envelopes==busy.envelopes,"busy event skips all child reads/updates");
    busy.event_busy=true;StartupSoundRing invalid_ring;invalid_ring.read=999;check(sound_voice_lifecycle_event_step(library,profile,busy,invalid_ring,{}, {},&e,&error)&&e.reentrant_skip&&e.ring.read==999,"outer reentrancy skips prefix/output-mode reads entirely");
    for(u16 countdown:{u16(0),u16(1),u16(4),u16(5),u16(0x8000),u16(0xffff)}){
        auto r=ring(1,countdown);check(sound_voice_lifecycle_event_step(library,profile,initial,r,output,acquisitions,&e,&error),"countdown fixture allocation");auto t=e.state;
        check(t.events.events[0].phase0c==(countdown?1:2),"phase0 tests source3A, not child wait5A");t.events.events[0].wait5a=55;
        check(sound_voice_lifecycle_event_step(library,profile,t,empty,output,{},&e,&error),"countdown next event pass");const auto& event=e.state.events.events[0];
        const u16 remaining=u16(countdown-4u);const bool expired=countdown&&std::bit_cast<s16>(remaining)<1;
        check(event.flags3a==(countdown?remaining:0)&&event.wait5a==2&&event.phase0c==(expired?3:countdown?1:2),"signed countdown wraps before comparison; wait5A independently recounts live children");
    }
    for(s32 stop:{0x10,0x20}){
        auto r=ring();r.packets[1].known=1;r.packets[1].bytes[0]=u8(stop);r.write=2;
        check(sound_voice_lifecycle_event_step(library,profile,initial,r,output,{},&e,&error)&&e.state.events.events[0].phase0c==3&&e.state.events.events[0].requested08==0x10&&!e.released_events,"phase0 stopped before allocation forces10, cleanup delayed");
        check(sound_voice_lifecycle_event_step(library,profile,e.state,e.ring,output,{},&e,&error)==false,"event/ring output alias refused without erasing state");
        auto t=e.state;auto end=e.ring;check(sound_voice_lifecycle_event_step(library,profile,t,end,output,{},&e,&error)&&e.released_events==1,"empty stopped phase3 released next pass");
    }
    // Source steal: old child keeps its generation and linked slot, but its
    // envelope must never issue stop/free against the new owner of that token.
    auto r=ring(2);std::array<SoundVoiceLifecycleAcquisition,4> stolen={acquire(0,0,0,100,1),acquire(0,1,1,101,1),acquire(1,0,2,100,2,1,0,3),acquire(1,1,3,103,2)};
    check(sound_voice_lifecycle_event_step(library,profile,initial,r,output,stolen,&e,&error)&&e.steals==1,"two-event allocation with exact stolen owner");
    state=e.state;obs=observations(state);check(sound_voice_lifecycle_callback_step(envelope,state,obs,&c,&error)&&c.updates[0].envelope.state.done&&!c.updates[0].envelope.stop_and_free_backend&&!c.updates[0].envelope.publish_parameters,"stolen callback never touches current backend owner");
    state=c.state;auto end=e.ring;check(sound_voice_lifecycle_event_step(library,profile,state,end,output,{},&e,&error)&&e.unlinked_children==1&&e.state.events.events[0].wait5a==1&&e.state.voices.voices[2].allocated,"unlink stolen child only; new live owner survives");
    auto invalid=acquisitions;invalid[1].event_generation=999;check(!sound_voice_lifecycle_event_step(library,profile,initial,commands,output,invalid,&e,&error)&&e.initialized==0&&initial.events.free_count==64,"late receipt mismatch rolls back prefix/allocation candidate");
    SoundVoiceLifecycleState overflow;overflow.generation=std::numeric_limits<u64>::max();check(!sound_voice_lifecycle_event_step(library,profile,overflow,commands,output,acquisitions,&e,&error),"native generation overflow refuses rather than aliases old handles");
    check(sound_voice_lifecycle_event_step(library,profile,initial,commands,output,acquisitions,&e,&error),"new candidate for callback alias");state=e.state;obs=observations(state);check(sound_voice_lifecycle_callback_step(envelope,state,obs,&c,&error),"callback independent output");const auto identity=sound_voice_lifecycle_identity(c.state,0);check(!sound_voice_lifecycle_callback_step(envelope,c.state,{},&c,&error)&&sound_voice_lifecycle_identity(c.state,0)==identity,"callback alias refused before output clear");
    std::printf("{\"sound_voice_lifecycle_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
