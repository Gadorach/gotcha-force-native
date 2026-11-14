#pragma once
#include <cstdlib>
#include "engine/gameplay/sound_pcm_source.hpp"
#include "engine/gameplay/sound_control_owner.hpp"
#include "engine/gameplay/sound_command_transport.hpp"
#include "tests/unit/sound_pcm_source_fixture.hpp"
#include "tests/unit/startup_sound_commands_fixture.hpp"
namespace sound_pcm_boundary_fixture {
struct Resources {
 gf::SoundVoiceLibrary library;gf::SoundVoiceProfile voice;gf::SoundEnvelopeProfile envelope;gf::SoundCommandBanks commands;
 bool load(std::string* error){using namespace gf;auto data=sound_cpu_backend_fixture::bank(true);data.tones[23]=100;data.tones[24]=127;data.tones[25]=255;const std::array<SoundVoiceBankInput,1> input={data.input()};auto v=sound_voice_fixture::profile(),e=sound_envelope_fixture::profile(),p=startup_sound_commands_fixture::profile(data.tones);StartupSoundCommandProfile profile;const std::array<SoundCommandBankInput,1> tones={SoundCommandBankInput{0,{data.tones.data(),data.tones.size()}}};return library.load(input,error)&&sound_voice_profile_load({v.data(),v.size()},&voice,error)&&envelope.load({e.data(),e.size()},error)&&startup_sound_command_load({p.data(),p.size()},&profile,error)&&commands.load(profile,tones,error)&&sound_command_voice_bindings_valid(commands,library);}
 gf::StartupSoundCommandQueue start(gf::s8 actor,std::string* error)const{using namespace gf;StartupSoundScalarCommand call;call.volume=127;call.pan=64;call.actor96=actor;StartupSoundCommandPlan out;if(!sound_command_plan(commands,{}, {true,true,0,4},std::span(&call,1),&out,error))std::abort();return out.candidate;}
};
struct Provider final:gf::SoundPcmBoundaryProvider {
 gf::SoundControlSnapshot snapshot;std::vector<gf::SoundPcmEventBoundary> data;std::vector<gf::u64> due;
 unsigned visible=0;bool invalid=false,bad_controls=false;mutable unsigned reads=0,lookups=0,control_reads=0;
 bool valid(std::string* error)const override{++reads;if(invalid||!snapshot.live()){if(error)*error="owned boundary provider: invalid source snapshot";return false;}return true;}
 gf::u32 available(const gf::SoundCpuControlState& state)const override{gf::u32 n=0;while(n<visible&&(due.empty()||n>=due.size()||due[n]<=state.parameters.lifecycle.callback+1))++n;return n;}
 bool boundary(gf::u32 ordinal,gf::SoundPcmEventBoundary* out,std::string* error)const override{++lookups;if(!ordinal||ordinal>data.size()){if(error)*error="owned boundary provider: missing ordinal";return false;}*out=data[ordinal-1];return true;}
 bool controls(const gf::SoundCpuControlState& state,gf::SoundVoiceControlObservation* controls,std::vector<gf::SoundVoiceBankGainObservation>* banks,std::string* error)const override{++control_reads;gf::SoundControlFrameInput input;if(!snapshot.frame(state.parameters.lifecycle.callback+1,&input,error))return false;*controls=input.controls;*banks=std::move(input.banks);if(bad_controls)controls->callback+=1;return true;}
};
inline Provider provider(const Resources& r,const gf::SoundControlSnapshot& snapshot,std::string* error){Provider p;p.snapshot=snapshot;gf::SoundControlFrameInput mode;if(!snapshot.frame(1,&mode,error))std::abort();p.data={{1,r.start(0,error),mode.event_mode},{2,r.start(1,error),mode.event_mode},{3,{},mode.event_mode}};p.visible=3;return p;}
} // namespace sound_pcm_boundary_fixture
