#include <cstdio>
#include <cstdlib>
#include "tests/unit/sound_envelope_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"voice envelope: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;auto bytes=sound_envelope_fixture::profile();std::string error;SoundEnvelopeProfile profile;check(profile.load({bytes.data(),bytes.size()},&error),"owned parametric profile");
    for(unsigned at=0;at<16;++at){auto bad=bytes;bad[at]^=128;SoundEnvelopeProfile rejected;check(!rejected.load({bad.data(),bad.size()},&error)&&!rejected.ready(),"header and source scalar bytes checked");}
    for(unsigned i=0;i<128;++i){auto bad=bytes;store_be32(bad.data()+16+i*4,0x8000001u);SoundEnvelopeProfile rejected;check(!rejected.load({bad.data(),bad.size()},&error),"table arithmetic bounds checked for every entry");}
    check(!profile.load({bytes.data(),527},&error)&&!profile.ready(),"failed profile reload invalidates prior binding");check(profile.load({bytes.data(),bytes.size()},&error),"explicit profile rebind");
    SoundEnvelopePlan p;SoundEnvelopeObservation o;o.backend_state_declared=true;o.backend_state=1;
    const u32 word=(10u<<25)|(7u<<19)|(20u<<13)|(3u<<6)|2u;
    check(sound_voice_envelope_step(profile,word,{},o,&p,&error)&&p.state==SoundEnvelopeState{1,10u<<20,false}&&p.start_backend&&p.publish_parameters,"phase0 falls through attack and starts backend once");
    check(p.sustain_level==(20u<<21)&&p.attack==(10u<<20)&&p.decay==(14u<<20)&&p.sustain==(6u<<20)&&p.release==(4u<<20),"one shared table with distinct bit-field indices");
    SoundEnvelopeState s{1,0x7fffffe,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state.phase==2&&p.state.level==0x7ffffff&&!p.start_backend,"attack crossing clamps and changes phase, no decay same callback");
    s={2,21u<<21,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state.phase==3&&p.state.level==(20u<<21),"decay clamps to authored sustain threshold");
    s={3,1,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state==SoundEnvelopeState{6,0,true}&&p.stop_and_free_backend&&!p.publish_parameters&&p.stop_checks==2,"downward sustain crosses zero and executes release in SAME callback");
    check(sound_voice_envelope_step(profile,word|0x1000u,s,o,&p,&error)&&p.state.phase==3&&p.state.level==1+(6u<<20)&&p.stop_checks==1,"sustain direction bit is not part of sustain-rate index");
    o.event_stop=1;o.requested=0x10;s={1,0x7ffffff,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state==SoundEnvelopeState{5,0x7ffffff,false}&&p.publish_parameters&&!p.stop_and_free_backend&&p.backend_reads==0,"stop request changes attack to fast release WITHOUT reducing this callback");
    s={4,0x7ffffff,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state.phase==5&&p.state.level==0x7ffffff-(4u<<20),"phase4 fast stop still subtracts normal release this callback");
    s={5,0x7ffffff,false};o.backend_state_declared=false;check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state.level==0x7ffffff-0x1555555u&&p.backend_reads==0,"phase5 fixed source decrement, no backend state read");
    s={5,0x1555555,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state==SoundEnvelopeState{6,0,true}&&p.stop_and_free_backend,"fast release equality completes");
    s={6,0,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state==s&&!p.publish_parameters&&!p.stop_and_free_backend,"terminal does not invent another backend free or done write");
    s={2,0x7000000,false};o={};o.stolen=true;check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state==SoundEnvelopeState{6,0x7000000,true}&&!p.stop_and_free_backend&&!p.publish_parameters&&p.backend_reads==0,"stolen marks done but never frees former backend token");
    o={};o.backend_state_declared=true;o.backend_state=0;s={1,123,false};check(sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p.state==SoundEnvelopeState{4,0,false}&&p.publish_parameters&&!p.stop_and_free_backend,"ended backend enters zero-level release; free is delayed");
    check(sound_voice_envelope_step(profile,word,{},o,&p,&error)&&p.state.phase==1&&p.state.level==(10u<<20)&&p.start_backend,"initial backend state0 does NOT kill new voice");
    o.backend_state_declared=false;check(!sound_voice_envelope_step(profile,word,{},o,&p,&error)&&p==SoundEnvelopePlan{},"even FIRST callback requires branch-local backend observation");
    o.event_stop=255;check(sound_voice_envelope_step(profile,word,{},o,&p,&error)&&p.state.phase==4&&p.start_backend&&p.backend_reads==0,"nonzero stop byte bypasses backend read and initial callback still starts");
    s={3,0x7ffffff,false};o={};o.backend_state_declared=true;o.backend_state=7;check(sound_voice_envelope_step(profile,word|0x1000u,s,o,&p,&error)&&p.state.level==0x7ffffff,"upward sustain saturates at full");
    check(sound_voice_envelope_step(profile,0,{3,0,false},o,&p,&error)&&p.state.phase==6&&p.stop_and_free_backend,"zero sustain/release rates still terminate if level already zero");
    for(unsigned phase=0;phase<7;++phase)for(unsigned kind=0;kind<4;++kind){s={u8(phase),123,false};o={};o.backend_state_declared=true;o.backend_state=kind&1;o.event_stop=u8(kind>>1);check(sound_voice_envelope_step(profile,word,s,o,&p,&error),"bounded phase/stop-state combinations");}
    s={7,0,false};check(!sound_voice_envelope_step(profile,word,s,o,&p,&error)&&p==SoundEnvelopePlan{},"unknown source phase refused, not silently marked completed");
    s={1,0x8000000,false};check(!sound_voice_envelope_step(profile,word,s,o,&p,&error),"live level out of represented source range");s={2,1,true};check(!sound_voice_envelope_step(profile,word,s,o,&p,&error),"incoherent done live-phase guard");
    o={};o.stolen=true;check(sound_voice_envelope_step(profile,word,{0,0xffffffff,false},o,&p,&error)&&p.state.level==0xffffffff,"stolen new slot does not read/reset uninitialized old level");check(sound_voice_envelope_step(profile,word,p.state,o,&p,&error)==false&&p.state.level==0xffffffff,"state/output alias refused without erasing input");
    std::printf("{\"sound_envelope_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
