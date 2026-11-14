#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/sound_control_owner.hpp"
#include "engine/gameplay/sound_command_transport.hpp"
#include "tests/unit/sound_pcm_source_fixture.hpp"
#include "tests/unit/startup_sound_commands_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"control owner/shared queue: %s\n",why);std::abort();}}
struct Resources {
 gf::SoundVoiceLibrary library;gf::SoundVoiceProfile voice;gf::SoundEnvelopeProfile envelope;gf::SoundCommandBanks commands;
 bool load(std::string* error){using namespace gf;auto data=sound_cpu_backend_fixture::bank(true);data.tones[23]=100;data.tones[24]=127;data.tones[25]=255;const std::array<SoundVoiceBankInput,1> voice_input={data.input()};if(!library.load(voice_input,error))return false;auto vb=sound_voice_fixture::profile();auto eb=sound_envelope_fixture::profile();if(!sound_voice_profile_load({vb.data(),vb.size()},&voice,error)||!envelope.load({eb.data(),eb.size()},error))return false;auto pb=startup_sound_commands_fixture::profile(data.tones);StartupSoundCommandProfile p;if(!startup_sound_command_load({pb.data(),pb.size()},&p,error))return false;const std::array<SoundCommandBankInput,1> input={SoundCommandBankInput{0,{data.tones.data(),data.tones.size()}}};return commands.load(p,input,error)&&sound_command_voice_bindings_valid(commands,library);}
};
struct Provider final:gf::SoundPcmFrameProvider {
 gf::SoundControlSnapshot snapshot;gf::StartupSoundRing captured_ring;bool legacy=false;mutable unsigned lookups=0;
 bool inputs(const gf::SoundCpuControlState& state,gf::SoundPcmFrameInputs* out,std::string* error)const override{
  using namespace gf;++lookups;const u64 callback=state.parameters.lifecycle.callback+1;*out={};
  if(legacy){out->controls=sound_voice_parameter_fixture::controls(callback);for(auto& aux:out->controls.aux)aux.present=false;out->banks.push_back(sound_voice_parameter_fixture::bank(0,callback));if(callback==1)out->event={true,{true,1}};}
  else{SoundControlFrameInput input;if(!snapshot.frame(callback,&input,error))return false;out->controls=input.controls;out->banks=std::move(input.banks);if(callback==1)out->event={true,input.event_mode};}
  if(callback==1)out->ring=captured_ring;
  return true;
 }
};
gf::StartupSoundRing lower(const Resources& resources,const gf::SoundControlSnapshot& snapshot){using namespace gf;std::string error;StartupSoundCommandObservation observation;check(snapshot.commands(&observation,&error),"immutable source command observation");StartupSoundScalarCommand cue;cue.cue=0;cue.volume=127;cue.pan=64;SoundCommandTransportPlan transport;check(sound_command_transport(resources.commands,{}, {},observation,std::span(&cue,1),&transport,&error)&&transport.forwarded==1,"source-weighted command uses same ring transport");return transport.ring;}
std::vector<std::array<gf::s16,2>> render(const Resources& resources,unsigned index,gf::f32 host_sfx,bool legacy){using namespace gf;std::string error;SoundControlOwner owner;const std::array<u8,1> bank={0};OptionSettings settings;settings.volume[1]=u8(index);check(owner.initialize(bank,settings,&error),"owned active settings owner");Provider provider;provider.snapshot=owner.snapshot();provider.captured_ring=lower(resources,provider.snapshot);provider.legacy=legacy;
 sound_pcm_source_fixture::Sink sink;PcmQueue queue(sink);PcmMixer mixer;check(queue.start()&&mixer.set_policy(false,{1,host_sfx,1}),"one shared fake sink/mixer");SoundPcmSource source;check(source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&source.pump(queue,mixer,provider,&error),"owner controls -> native -> wide converter -> existing shared queue");check(queue.submitted()==2048&&source.snapshot().admitted_device_frames==2048&&provider.lookups>0,"only successful shared submissions commit source/device-frame state");sink.consume(2048);bool nonzero=false;for(auto sample:sink.heard)nonzero|=sample[0]!=0||sample[1]!=0;check(nonzero,"actual queued PCM, not success-zero");return sink.heard;}
}
int main(){
 using namespace gf;std::string error;Resources resources;check(resources.load(&error),"owned compressed sample/bank resources");
 for(unsigned index=0;index<5;++index){const auto current=render(resources,index,1,false),again=render(resources,index,.2f,false),legacy=render(resources,index,1,true);check(current==again,"source-weighted SFX does not get a second host preference factor");check(current==legacy,"new initialized control producer preserves prior declared-empty-controls PCM");}
 SoundControlOwner owner;const std::array<u8,1> bank={0};check(owner.initialize(bank,{},&error),"fresh owner for submit edges");Provider provider;provider.snapshot=owner.snapshot();provider.captured_ring=lower(resources,provider.snapshot);
 {sound_pcm_source_fixture::Sink sink;sink.present=false;PcmQueue queue(sink);PcmMixer mixer;SoundPcmSource source;check(queue.start()&&source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&source.pump(queue,mixer,provider,&error)&&!provider.lookups&&source.snapshot().generated_control_frames==0&&source.snapshot().event_pass==0,"no device invokes no provider/control/native work");}
 {sound_pcm_source_fixture::Sink sink;sink.fail_at=1;PcmQueue queue(sink);PcmMixer mixer;SoundPcmSource source;check(queue.start()&&source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&!source.pump(queue,mixer,provider,&error)&&queue.failed()&&source.failed()&&source.snapshot().admitted_device_frames==0&&source.snapshot().event_pass==0,"rejected first submission rolls back automatic voices/control/ring-source candidate");const auto calls=provider.lookups;check(!source.pump(queue,mixer,provider,&error)&&provider.lookups==calls&&sink.submissions==1,"rejected source latched, no callback/packet replay");}
 {sound_pcm_source_fixture::Sink sink;sink.fail_at=2;PcmQueue queue(sink);PcmMixer mixer;SoundPcmSource source;check(queue.start()&&source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&!source.pump(queue,mixer,provider,&error)&&queue.submitted()==1024&&source.snapshot().admitted_device_frames==1024&&source.snapshot().generated_control_frames>0,"later failure retains earlier accepted source state and rolls back only failing block");}
 {sound_pcm_source_fixture::Sink sink;sink.fail_at=1;sink.accept_but_fail=true;PcmQueue queue(sink);PcmMixer mixer;SoundPcmSource source;check(queue.start()&&source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&!source.pump(queue,mixer,provider,&error)&&source.snapshot().admitted_device_frames==0&&sink.frames_available()==1024,"uncertain acceptance is not committed or replayed, accepted sink bytes honestly retained");}
 check(!owner.require_auxiliary_effect(1,&error)&&owner.failed(),"new source effect request cannot be faked as no effect");
 {sound_pcm_source_fixture::Sink sink;PcmQueue queue(sink);PcmMixer mixer;SoundPcmSource source;check(queue.start()&&source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&!source.pump(queue,mixer,provider,&error)&&sink.submissions==0&&source.snapshot().generated_control_frames==0,"invalidated control scope prevents source work/submit after unsupported effect request");}
 std::printf("{\"sound_control_queue_owned_checks\":%u,\"settings_levels\":5,\"queued_PCM_witness_cases\":15,\"sink_is_fake\":true,\"all_passed\":true}\n",checks);
}
