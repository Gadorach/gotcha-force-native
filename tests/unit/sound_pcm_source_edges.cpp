#include <cstdio>
#include <cstdlib>
#include "tests/unit/sound_pcm_source_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"shared SFX edges: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace sound_pcm_source_fixture;std::string error;Resources resources;check(resources.load(&error),"owned source resources");
    for(unsigned fail_at:{1u,2u})for(bool uncertain:{false,true}){SoundPcmSource source;Provider p;Sink sink;sink.fail_at=fail_at;sink.accept_but_fail=uncertain;PcmQueue queue(sink);PcmMixer mix;check(source.load(resources.library,resources.voice,resources.envelope,44100,&error)&&queue.start(),"partial admission fixture");
        check(!source.pump(queue,mix,p,&error)&&source.failed()&&queue.failed()&&source.snapshot().admitted_device_frames==u64(fail_at-1)*1024&&queue.submitted()==u64(fail_at-1)*1024,"only successful prior blocks commit; rejected/unknown block never admitted");
        const auto snapshot=source.snapshot();const auto submissions=sink.submissions;check(!source.pump(queue,mix,p,&error)&&source.snapshot().conversion==snapshot.conversion&&source.snapshot().backend.subframes==snapshot.backend.subframes&&sink.submissions==submissions,"latched transport failure cannot decode/regenerate/retry source");
    }
    // Failed preparation after this producer has staged but another source
    // refuses must discard all speculative SFX work as well.
    SoundPcmSource source;Provider p;Sink sink;PcmQueue queue(sink);PcmMixer mixer;check(source.load(resources.library,resources.voice,resources.envelope,48000,&error)&&queue.start(),"multiple-source failure fixture");bool complete_called=false;
    check(!queue.pump(mixer,[&](unsigned n,PcmMixer& m){if(!source.prepare(n,m,p,&error))return false;return false;},[&](bool ok){complete_called=true;check(!ok,"failed composite prepare acknowledged once as rejected");return source.complete(ok);})&&complete_called&&source.failed()&&source.snapshot().generated_control_frames==0&&sink.submissions==0,"later composite source failure discards prior source candidate before any sink transaction");
    // New explicit wide metadata survives selective cut/restore without reapplying
    // current options or replaying any producer prepare/complete callbacks.
    Sink cut_sink;PcmQueue cut(cut_sink);PcmMixer mix;unsigned prepares=0,commits=0;check(mix.set_policy(false,{1,0.001f,1})&&cut.start(),"wide journal fixture");
    check(cut.pump(mix,[&](unsigned n,PcmMixer& m){++prepares;return m.add(AudioBus::Bgm,std::vector<s16>(n*2,-20000))&&m.add_source_weighted_sfx(std::vector<s64>(n*2,s64(40000)*65536));},[&](bool ok){commits+=ok;return ok;})&&cut_sink.queued.front()[0]==20000,"original shared sum uses wide SFX and cancellation before final clip");
    cut_sink.consume(71);check(mix.set_policy(true,{0,0,0})&&cut.cut(AudioBus::Bgm)&&cut_sink.queued.front()[0]==32767&&prepares==2&&commits==2&&cut.restored()==1977,"restore retains wide SFX metadata/source weight and old mono, not new host policy");
    check(cut.cut(AudioBus::Voice)&&cut_sink.queued.front()[0]==32767&&prepares==2&&commits==2,"repeated unrelated bus cuts preserve exactly retained weighted contribution");check(cut.cut(AudioBus::Sfx)&&cut_sink.queued.empty()&&cut.journal_frames()==0,"cut all remaining wide bus drops pending data, never restarts producer");
    // Source-owned mode2/3 may produce zero dry output. Host only refuses actual
    // nonzero unsupported lanes, not a guessed gain or hidden fake return.
    PcmMixer invalid;check(invalid.begin(1)&&invalid.add_source_weighted_sfx(std::array<s64,2>{0,0})&&!invalid.set_policy(false,{1,1,1}),"source weighting locks shared-block policy after first source");
    SoundPcmSource unloaded;check(!unloaded.pump(queue,mixer,p,&error),"unloaded producer cannot manufacture an empty accepted audio clock");
    std::printf("{\"shared_sfx_pcm_edge_checks\":%u,\"all_passed\":true}\n",checks);
}
