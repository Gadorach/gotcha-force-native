#include <cstdio>
#include <cstdlib>
#include "tests/unit/sound_voice_parameter_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"voice parameter edges: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace sound_voice_parameter_fixture;using namespace sound_voice_lifecycle_fixture;std::string error;auto b=sound_voice_fixture::profile();SoundVoiceProfile profile;check(sound_voice_profile_load({b.data(),b.size()},&profile,&error),"voice profile");auto eb=sound_envelope_fixture::profile();SoundEnvelopeProfile envelope;check(envelope.load({eb.data(),eb.size()},&error),"owned envelope profile");sound_voice_fixture::Bank data;SoundVoiceLibrary library;const std::array<SoundVoiceBankInput,1> input={data.input()};check(library.load(input,&error),"owned library");
    SoundVoiceParameterState fresh;SoundVoiceParameterEventPlan event;const StartupSoundRing empty;auto r=ring();std::array<SoundVoiceLifecycleAcquisition,2> acquisitions={acquire(0,0,0,100),acquire(0,1,1,101)};
    check(sound_voice_parameter_event_step(library,profile,fresh,r,{true,1},acquisitions,&event,&error),"initial two voices");auto state=event.state;SoundVoiceParameterPlan plan;
    for(unsigned callback=1;callback<=3;++callback){auto c=controls(callback);std::array<SoundVoiceBankGainObservation,1> gains={bank(0,callback)};auto backend=observations(state.lifecycle,0);check(sound_voice_parameter_step(profile,envelope,state,c,gains,backend,&plan,&error),"initial/end/release callback candidates");state=plan.state;}
    check(state.lifecycle.voices.voices[0].done24&&state.voices[0].generation==2&&state.voices[0].mix[0]==16384,"completed child retains parameter cache until event cleanup");const auto ramps=state.ramps;
    check(sound_voice_parameter_event_step(library,profile,state,empty,{true,1},{},&event,&error)&&event.unlinked==2&&event.released==1,"cleanup consumes done children");state=event.state;
    check(state.voices[0].generation==0&&state.voices[0].src_q16==0&&state.voices[0].mix==std::array<u16,18>{}&&state.ramps==ramps,"retired cache erased but global ramp not restarted per sound");
    acquisitions={acquire(0,0,1,100,4,3),acquire(0,1,0,101,4,3)};
    check(sound_voice_parameter_event_step(library,profile,state,r,{true,2},acquisitions,&event,&error),"LIFO reuse with new generations and mode2");state=event.state;
    check(state.voices[1].generation==5&&state.voices[0].generation==6&&!state.voices[0].pitch_valid&&state.voices[0].mix==std::array<u16,18>{}&&state.ramps==ramps,"same numeric tokens cannot inherit old SRC/pan/mix cache");
    auto c=controls(4);std::array<SoundVoiceBankGainObservation,1> gains={bank(0,4)};auto backend=observations(state.lifecycle,0);check(sound_voice_parameter_step(profile,envelope,state,c,gains,backend,&plan,&error)&&plan.packets[0].mix==std::array<u16,18>{}&&plan.packets[0].pitch_write&&plan.packets[1].pitch_write,"mode2 fresh cache retains zero dry pair; pitch initialized independently");
    auto wrong=state;wrong.voices[0].generation=2;check(!sound_voice_parameter_step(profile,envelope,wrong,c,gains,backend,&plan,&error),"retired cache generation rejected");
    auto doubled=std::array<SoundVoiceBankGainObservation,2>{bank(0,4),bank(0,4)};check(!sound_voice_parameter_step(profile,envelope,state,c,doubled,backend,&plan,&error),"duplicate bank gain identity rejected");
    auto stale=gains;stale[0].declared=false;check(!sound_voice_parameter_step(profile,envelope,state,c,stale,backend,&plan,&error),"undeclared gain bank rejected");
    wrong=state;wrong.ramps.master=8129;check(!sound_voice_parameter_step(profile,envelope,wrong,c,gains,backend,&plan,&error),"out-of-domain source master ramp not clamped as fabricated history");
    wrong=state;wrong.lifecycle.events.events[0].program38=128;for(auto& v:wrong.lifecycle.voices.voices)if(v.allocated)v.seed.program=128;
    check(!sound_voice_parameter_step(profile,envelope,wrong,c,gains,observations(wrong.lifecycle,0),&plan,&error),"bank program-volume table bounds remain128, no neighboring-state read");
    // Pan and envelope/source-state constraints are separate from unsigned raw
    // mix products. Exercise full16-bit source factor values with wrap-before-
    // division, not widened arithmetic followed by a single final division.
    using namespace sound_voice_parameters_detail;std::array<u16,18> previous{};previous[0]=previous[2]=65535;
    for(unsigned factor=0;factor<65536;++factor){std::array<std::array<u16,4>,2> factors={std::array<u16,4>{u16(factor),65535,65535,65535},std::array<u16,4>{65535,u16(factor),32767,65535}};
        auto result=mix(previous,3,0,factors);
        for(unsigned bus=0;bus<2;++bus){u64 expected=65535;for(auto value:factors[bus])expected=((expected*value)&0xffffffffu)/32767u;check(result[4+bus*4]==u16(expected)&&result[6+bus*4]==u16(expected),"each multiply truncates32 bits and each divide truncates before next stage");}
    }
    std::array<u8,6> high;high.fill(254);const u8 wrapped=volume(254,high);check(wrapped!=127&&wrapped!=254,"effective volume is a wrapped source byte, not native127 saturation");
    std::printf("{\"sound_voice_parameter_edge_checks\":%u,\"all_passed\":true}\n",checks);
}
