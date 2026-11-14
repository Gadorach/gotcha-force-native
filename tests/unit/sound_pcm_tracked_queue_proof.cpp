#include <cstdio>
#include <cstdlib>
#include "tests/unit/sound_pcm_boundary_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"tracked queue proof: %s\n",why);std::abort();}}
struct Provider final:gf::SoundPcmBoundaryProvider {
 sound_pcm_boundary_fixture::Provider input;std::shared_ptr<const gf::SoundPcmBoundaryBinding> token;
 bool valid(std::string* e)const override{return input.valid(e);}gf::u32 available(const gf::SoundCpuControlState& s)const override{return input.available(s);}bool boundary(gf::u32 n,gf::SoundPcmEventBoundary* o,std::string* e)const override{return input.boundary(n,o,e);}bool controls(const gf::SoundCpuControlState& s,gf::SoundVoiceControlObservation* c,std::vector<gf::SoundVoiceBankGainObservation>* b,std::string* e)const override{return input.controls(s,c,b,e);}std::shared_ptr<const gf::SoundPcmBoundaryBinding> binding()const override{return token;}
};
struct ClosingSink final:sound_pcm_source_fixture::Sink {
 std::shared_ptr<const gf::sound_pcm_boundary_detail::ProducerEpoch>* producer=nullptr;
 bool try_submit(const gf::s16* p,std::size_t n)override{const bool result=sound_pcm_source_fixture::Sink::try_submit(p,n);if(result&&producer)producer->reset();return result;}
};
}
int main(){
 using namespace gf;std::string error;sound_pcm_boundary_fixture::Resources resources;check(resources.load(&error),"owned native resources");SoundControlOwner controls;const std::array<u8,1> ids={0};check(controls.initialize(ids,{},&error),"initialized native controls");
 for(bool manual_sink:{false,true}){SoundPcmSource source;check(source.load(resources.library,resources.voice,resources.envelope,48000,&error),"fresh source");auto binding=source.create_boundary_binding(&error);auto producer=std::make_shared<const sound_pcm_boundary_detail::ProducerEpoch>();check(binding&&binding->claim(producer,&error),"producer binding");Provider provider;provider.token=binding;provider.input=sound_pcm_boundary_fixture::provider(resources,controls.snapshot(),&error);PcmMixer mix;check(mix.begin(64)&&source.prepare_tracked_boundaries(64,mix,provider,&error),"tracked speculative block");if(manual_sink){std::vector<s16> pcm;check(mix.finish(&pcm),"manual block encode");sound_pcm_source_fixture::Sink sink;check(sink.try_submit(pcm.data(),64)&&sink.frames_available()==64,"manual owned sink really accepted bytes");}
  check(!source.complete(true)&&source.failed()&&!source.boundary_receipt().valid()&&source.snapshot().event_boundary==0&&source.snapshot().admitted_device_frames==0,"public boolean completion cannot grant tracked queue authority, with or without manual sink bytes");}
 {SoundPcmSource source;check(source.load(resources.library,resources.voice,resources.envelope,48000,&error),"queue-close source");auto token=source.create_boundary_binding(&error);auto producer=std::make_shared<const sound_pcm_boundary_detail::ProducerEpoch>();check(token->claim(producer,&error),"queue-close producer");Provider p;p.token=token;p.input=sound_pcm_boundary_fixture::provider(resources,controls.snapshot(),&error);ClosingSink sink;sink.producer=&producer;PcmQueue q(sink);PcmMixer mix;check(q.start()&&!source.pump_tracked_boundaries(q,mix,p,&error)&&sink.frames_available()==1024&&q.failed()&&source.failed()&&!source.boundary_receipt().valid()&&source.snapshot().event_boundary==0,"actual queue accepts bytes, then producer close prevents stale tracked commit/receipt");}
 {SoundPcmSource source;check(source.load(resources.library,resources.voice,resources.envelope,48000,&error),"valid queued source");auto token=source.create_boundary_binding(&error);auto producer=std::make_shared<const sound_pcm_boundary_detail::ProducerEpoch>();check(token->claim(producer,&error),"valid queued producer");Provider p;p.token=token;p.input=sound_pcm_boundary_fixture::provider(resources,controls.snapshot(),&error);sound_pcm_source_fixture::Sink sink;PcmQueue q(sink);PcmMixer mix;check(q.start()&&source.pump_tracked_boundaries(q,mix,p,&error)&&q.submitted()==2048&&source.boundary_receipt().valid()&&source.boundary_receipt().sequence()==2,"only private successful shared-queue completion creates tracked receipt");}
 std::printf("{\"tracked_queue_completion_proof_owned_checks\":%u,\"public_bool_not_delivery_authority\":true,\"all_passed\":true}\n",checks);
}
