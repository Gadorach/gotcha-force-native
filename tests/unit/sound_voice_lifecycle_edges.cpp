#include <cstdio>
#include <cstdlib>
#include "tests/unit/sound_voice_lifecycle_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"voice lifecycle edges: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace sound_voice_lifecycle_fixture;std::string error;SoundVoiceProfile profile;auto b=sound_voice_fixture::profile();check(sound_voice_profile_load({b.data(),b.size()},&profile,&error),"voice scalar profile");const StartupSoundRing empty;const SoundVoiceLifecycleEventObservation mode{true,1};
    unsigned scenarios=0;
    for(unsigned children=1;children<=4;++children){sound_voice_fixture::Bank bank(children);SoundVoiceLibrary library;const std::array<SoundVoiceBankInput,1> input={bank.input()};check(library.load(input,&error),"owned region count");
        for(unsigned events=1;events<=5;++events){SoundVoiceLifecycleState fresh;std::vector<SoundVoiceLifecycleAcquisition> acquired;
            for(unsigned event=0;event<events;++event)for(unsigned i=0;i<children;++i){const unsigned slot=event*children+i;acquired.push_back(acquire(u16(event),u16(i),u8(slot),slot+100,event+1));}
            auto packets=ring(events);SoundVoiceLifecycleEventPlan base;check(sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,acquired,&base,&error),"multi-event/child allocation");
            for(unsigned victim=0;victim<events;++victim)for(unsigned mask=0;mask<(1u<<children);++mask){auto s=base.state;unsigned done=0;for(unsigned i=0;i<children;++i)if(mask&(1u<<i)){const unsigned slot=victim*children+i;s.voices.voices[slot].done24=true;s.envelopes[slot]={6,0,true};++done;}
                SoundVoiceLifecycleEventPlan p;check(sound_voice_lifecycle_event_step(library,profile,s,empty,mode,{},&p,&error)&&p.unlinked_children==done&&p.released_events==unsigned(done==children),"head/middle/tail/multiple child removal and event release");
                check(p.state.voices.free_count==64-events*children+done&&p.state.events.free_count==64-events+unsigned(done==children)&&sound_voice_lifecycle_valid(p.state),"derived linked/free topology and native generations");
                const auto& event=p.state.events.events[victim];check(event.count58==children&&event.wait5a==children-done,"historical initialized count vs fresh live-child recount");
                unsigned free_index=64-events*children;for(unsigned i=0;i<children;++i){const unsigned slot=victim*children+i;if(mask&(1u<<i)){check(p.state.voices.free[free_index++]==slot&&!p.state.voice_generation[slot],"source traversal-order free-stack push and generation retirement");}else check(p.state.voice_generation[slot]==s.voice_generation[slot],"unremoved child generation retained");}
                ++scenarios;
            }
        }
    }
    // A stolen child can be newly created earlier in the SAME region expansion.
    sound_voice_fixture::Bank bank;SoundVoiceLibrary library;std::array<SoundVoiceBankInput,1> input={bank.input()};check(library.load(input,&error),"owned overlapping two-region resource");SoundVoiceLifecycleState fresh;SoundVoiceLifecycleEventPlan p;auto packets=ring();std::array<SoundVoiceLifecycleAcquisition,2> steals={acquire(0,0,0,100),acquire(0,1,1,100,1,1,0,2)};
    check(sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,steals,&p,&error)&&p.steals==1&&p.state.voices.voices[0].stolen25&&p.state.voice_generation[0]==2&&p.state.voice_generation[1]==3,"same-call generation preview binds exact stolen generation");
    for(u64 generation:{u64(0),u64(1),u64(3),u64(999)}){auto bad=steals;bad[1].stolen_generation=generation;check(!sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,bad,&p,&error)&&p.initialized==0,"missing/stale/neighbor stolen generation refuses whole candidate");}
    auto bad=steals;bad[0].stolen_generation=2;check(!sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,bad,&p,&error),"spurious steal generation cannot accompany an ordinary acquisition");
    bad=steals;bad[1].event_pass=2;check(!sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,bad,&p,&error),"stale acquisition pass refuses");
    // Sixty-four children fill software capacity; the next event gets no fake
    // backend receipt and is removed only on its next phase2 cleanup pass.
    sound_voice_fixture::Bank four(4);input={four.input()};check(library.load(input,&error),"owned four-region resource");packets=ring(17);std::vector<SoundVoiceLifecycleAcquisition> all;
    for(unsigned event=0;event<16;++event)for(unsigned i=0;i<4;++i){const unsigned slot=event*4+i;all.push_back(acquire(u16(event),u16(i),u8(slot),slot+1,event+1));}
    check(sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,all,&p,&error)&&p.initialized==64&&p.state.events.events[16].allocated&&p.state.events.events[16].phase0c==2&&!p.state.voices.free_count,"pool-full event waits for source cleanup instead of premature completion");
    auto full=p.state;check(sound_voice_lifecycle_event_step(library,profile,full,empty,mode,{},&p,&error)&&p.released_events==1&&p.unlinked_children==0&&p.state.voices.free_count==0,"next event pass removes only empty pool-full event");
    // All-zero failures: first pass retains phase2, next pass releases event.
    input={bank.input()};check(library.load(input,&error),"fresh two-region library");packets=ring();std::array<SoundVoiceLifecycleAcquisition,2> failed={acquire(0,0,0,0),acquire(0,1,0,0)};
    check(sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,failed,&p,&error)&&p.initialized==0&&p.state.generation==1&&p.state.events.events[0].phase0c==2,"failed acquisition does not create a voice generation");
    auto zero=p.state;check(sound_voice_lifecycle_event_step(library,profile,zero,empty,mode,{},&p,&error)&&p.released_events==1,"all-failed event cleanup delayed to next pass");
    auto absent=bank;store_be32(absent.header.data()+48,0xffffffff);input={absent.input()};check(library.load(input,&error)&&sound_voice_lifecycle_event_step(library,profile,fresh,packets,mode,{},&p,&error)&&p.released_events==1&&p.state.events.free_count==64,"invalid-program flag releases no-child event in source phase0 pass");
    std::printf("{\"sound_voice_lifecycle_edge_checks\":%u,\"removal_scenarios\":%u,\"all_passed\":true}\n",checks,scenarios);
}
