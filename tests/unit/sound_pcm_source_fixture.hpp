#pragma once
#include <deque>
#include "engine/gameplay/sound_pcm_source.hpp"
#include "tests/unit/sound_cpu_backend_fixture.hpp"
namespace sound_pcm_source_fixture {
struct Sink:gf::IAudioBackend {
    bool present=true;unsigned submissions=0,fail_at=0;bool accept_but_fail=false;gf::u64 consumed=0,discarded=0;
    std::deque<std::array<gf::s16,2>> queued;std::vector<std::array<gf::s16,2>> heard;
    bool open(unsigned,unsigned)override{return true;}void close()override{present=false;}
    bool available()const override{return present;}void submit(const gf::s16*,std::size_t)override{std::abort();}
    gf::u64 frames_consumed()const override{return consumed;}gf::u64 frames_available()const override{return queued.size();}gf::u64 frames_discarded()const override{return discarded;}
    bool try_submit(const gf::s16* p,std::size_t n)override{++submissions;const bool fail=fail_at&&submissions==fail_at;if(!fail||accept_but_fail)for(unsigned i=0;i<n;++i)queued.push_back({p[i*2],p[i*2+1]});return !fail;}
    bool discard_queued()override{std::abort();}bool discard_queued_exact(gf::u64* n)override{if(!n)return false;*n=queued.size();discarded+=*n;queued.clear();return true;}
    void consume(unsigned n){while(n--&&!queued.empty()){heard.push_back(queued.front());queued.pop_front();++consumed;}}
};
struct Provider:gf::SoundPcmFrameProvider {
    mutable unsigned calls=0;unsigned stop_at=20,refuse_at=0;bool aux=false;gf::u16 cue=0;
    bool inputs(const gf::SoundCpuControlState& state,gf::SoundPcmFrameInputs* out,std::string* error)const override{
        using namespace gf;++calls;const auto callback=state.parameters.lifecycle.callback+1;if(callback==refuse_at){if(error)*error="owned provider refusal";return false;}
        *out={};out->controls=sound_voice_parameter_fixture::controls(callback);for(auto& a:out->controls.aux)a.present=aux;out->controls.aux_flags=aux?3:0;out->event={true,{true,1}};
        auto gains=sound_voice_parameter_fixture::bank(u8(cue/128),callback);out->banks.push_back(gains);
        if(callback==1){out->ring=sound_voice_lifecycle_fixture::ring();store_be32(out->ring.packets[0].bytes.data(),0x60000000u|u32(cue/128)<<16|u32(cue%128)<<8);}
        if(callback==stop_at){out->ring.write=1;auto& p=out->ring.packets[0];p.known=1;p.bytes[0]=0x10;}
        return true;
    }
};
struct Resources {
    gf::SoundVoiceLibrary library;gf::SoundVoiceProfile voice;gf::SoundEnvelopeProfile envelope;
    bool load(std::string* error){auto data=sound_cpu_backend_fixture::bank(true);std::array<gf::SoundVoiceBankInput,1> inputs={data.input()};if(!library.load(inputs,error))return false;auto vb=sound_voice_fixture::profile();auto eb=sound_envelope_fixture::profile();return gf::sound_voice_profile_load({vb.data(),vb.size()},&voice,error)&&envelope.load({eb.data(),eb.size()},error);}
};
} // namespace sound_pcm_source_fixture
