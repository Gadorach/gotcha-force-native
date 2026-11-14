#include <cstdio>
#include <cstdlib>
#include <deque>
#include <limits>
#include "engine/audio/pcm_queue.hpp"
#include "engine/gameplay/credits_music.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"PCM queue: %s\n",why);std::abort();}}
struct Sink:gf::IAudioBackend{
    bool present=true,reject_submit=false,reject_clear=false,exact=true,eager=false;unsigned submits=0,clears=0,race=0;gf::u64 consumed=0,dropped=0;
    std::deque<std::array<gf::s16,2>> queued;std::vector<std::array<gf::s16,2>> played;
    bool open(unsigned,unsigned)override{return true;}void close()override{present=false;queued.clear();}
    bool available()const override{return present;}void submit(const gf::s16*,std::size_t)override{std::abort();}
    gf::u64 frames_consumed()const override{return consumed;}gf::u64 frames_available()const override{return queued.size();}gf::u64 frames_discarded()const override{return dropped;}
    void consume(unsigned n){while(n--&&!queued.empty()){played.push_back(queued.front());queued.pop_front();++consumed;}}
    bool try_submit(const gf::s16* p,std::size_t n)override{++submits;if(reject_submit)return false;for(unsigned i=0;i<n;++i)queued.push_back({p[i*2],p[i*2+1]});if(eager)consume(unsigned(n));return true;}
    bool discard_queued()override{std::abort();} // shared owner must not use racy legacy clear
    bool discard_queued_exact(gf::u64* n)override{++clears;if(!exact||reject_clear||!n)return false;consume(race);race=0;*n=queued.size();dropped+=*n;queued.clear();return true;}
};
gf::AdxAudio clip(gf::s16 value){gf::AdxAudio c;c.rate=48000;c.channels=2;c.frames=64;c.loop=true;c.loop_end=64;c.samples.assign(128,value);return c;}
}
int main(){
    Sink sink;gf::PcmMixer mixer;gf::PcmQueue queue(sink);unsigned prepares=0,commits=0,position=0;
    auto prepare=[&](unsigned n,gf::PcmMixer& mix){++prepares;std::vector<gf::s16> bgm(n*2,30000),fx(n*2);
        for(unsigned i=0;i<n;++i){fx[i*2]=gf::s16(20000+(position+i)%1000);fx[i*2+1]=gf::s16(21000+(position+i)%1000);}position+=n;
        return mix.add(gf::AudioBus::Bgm,bgm)&&mix.add(gf::AudioBus::Sfx,fx);};
    auto complete=[&](bool ok){if(ok)++commits;return ok;};
    check(queue.start()&&queue.pump(mixer,prepare,complete)&&sink.queued.size()==2048&&prepares==2&&commits==2,"shared queue fills native watermark in bounded chunks");
    check(sink.queued[0][0]==32767&&sink.queued[0][1]==32767,"original mixed queue clips sum once");
    sink.consume(1025);sink.race=7;
    check(queue.cut(gf::AudioBus::Bgm)&&sink.queued.size()==1016&&queue.restored()==1016,"precise paused discard, not racy size snapshot, selects surviving tail");
    check(sink.queued.front()[0]==20032&&sink.queued.front()[1]==21032&&sink.queued.back()[0]==20047,"surviving SFX recovered from unclipped bus history, never replay delivered prefix");
    check(prepares==2&&commits==2&&position==2048&&queue.generated()==2048&&queue.submitted()==3064&&queue.discarded()==1016,"restoration doesn't decode, admit sources or count generated frames again");
    check(queue.clear()&&sink.queued.empty()&&queue.journal_frames()==0&&queue.restored()==1016,"explicit global clear abandons every bus without restore");
    check(!queue.cut(gf::AudioBus(3))&&!queue.failed(),"invalid bus refuses without effects");
    Sink mono_sink;gf::PcmQueue mono(mono_sink);gf::PcmMixer policy;unsigned blocks=0;
    auto mixed=[&](unsigned n,gf::PcmMixer& mix){++blocks;std::vector<gf::s16> a(n*2),b(n*2);for(unsigned i=0;i<n;++i){a[i*2]=2000;a[i*2+1]=4000;b[i*2]=10000;b[i*2+1]=-2000;}
        return mix.add(gf::AudioBus::Bgm,a)&&mix.add(gf::AudioBus::Sfx,b);};
    check(policy.set_policy(true,{0.5f,0.25f,1})&&mono.start()&&mono.pump(policy,mixed,[](bool ok){return ok;}),"mono/gain policy enters journal");
    check(policy.set_policy(false,{1,1,1})&&mono.cut(gf::AudioBus::Bgm)&&mono_sink.queued.front()[0]==1000&&mono_sink.queued.front()[1]==1000,"restore retains old mono and gain, not current policy or twice-applied gain");
    check(mono.cut(gf::AudioBus::Voice)&&mono_sink.queued.front()[0]==1000&&mono.restored()==4096,"repeated unrelated bus cut restores same surviving timeline");
    Sink empty_sink;gf::PcmQueue empty(empty_sink);gf::PcmMixer e;unsigned calls=0;
    check(empty.start()&&empty.pump(e,[&](unsigned,gf::PcmMixer&){++calls;return true;},[](bool ok){return ok;})&&empty_sink.submits==0&&calls==1,"no active sources don't fabricate submitted silence");
    Sink absent;absent.present=false;gf::PcmQueue quiet(absent);calls=0;
    check(quiet.start()&&quiet.pump(e,[&](unsigned,gf::PcmMixer&){++calls;return true;},[](bool){std::abort();return false;})&&calls==0&&quiet.generated()==0,"no device doesn't advance or complete sources");
    Sink foreign;foreign.queued.push_back({1,1});gf::PcmQueue unowned(foreign);check(!unowned.start()&&foreign.clears==0&&foreign.queued.size()==1,"preexisting foreign queue refused, not cleared or adopted");
    Sink broken;gf::PcmQueue fail(broken);unsigned failed_complete=0;
    auto constant=[](unsigned n,gf::PcmMixer& mix){return mix.add(gf::AudioBus::Sfx,std::vector<gf::s16>(n*2,111));};
    check(fail.start()&&fail.pump(e,constant,[](bool ok){return ok;}),"owned failure fixture starts");broken.reject_submit=true;
    const auto before=broken.submits;
    check(!fail.cut(gf::AudioBus::Bgm)&&fail.failed()&&broken.submits==before+1&&broken.queued.empty(),"failed restoration latches after clear, not replayed");
    check(!fail.cut(gf::AudioBus::Bgm)&&!fail.pump(e,constant,[&](bool){++failed_complete;return false;})&&broken.submits==before+1&&failed_complete==0,"failed queue blocks every retry/callback");
    Sink legacy;legacy.exact=false;gf::PcmQueue refused(legacy);
    check(refused.start()&&refused.pump(e,constant,[](bool ok){return ok;})&&!refused.cut(gf::AudioBus::Bgm)&&refused.failed()&&legacy.queued.size()==2048,"missing exact-discard capability refuses, never falls back to legacy clear");
    Sink submit;submit.reject_submit=true;gf::PcmQueue no_replay(submit);calls=0;
    check(no_replay.start()&&!no_replay.pump(e,constant,[&](bool ok){check(!ok,"rejected source receives failure once");++calls;return false;})&&no_replay.failed()&&calls==1&&no_replay.generated()==1024&&no_replay.submitted()==0,"generated rejected new block isn't accepted or replayed");
    Sink eager;eager.eager=true;gf::PcmQueue bounded(eager);
    check(bounded.start()&&bounded.pump(e,constant,[](bool ok){return ok;})&&bounded.generated()==4096&&bounded.journal_frames()==2048,"eager sink still respects work and journal budgets");
    Sink music_sink;gf::CreditsMusic music;gf::PcmQueue owner(music_sink);gf::MusicCue switched;
    check(music.load(clip(8000),clip(12000),48000,{4,8,1},1)&&music.set_active(true)&&owner.start(),"original two-cue source uses source-only ownership API");
    auto step=[&](){return music.advance(music_sink.available(),&switched)&&(switched==gf::MusicCue::None||owner.cut(gf::AudioBus::Bgm))&&owner.pump(e,[&](unsigned n,gf::PcmMixer& mix){return music.prepare(n,mix);},[&](bool ok){return music.complete(ok);});};
    check(step()&&music.accepted()==2048&&music_sink.queued.front()[0]==8000,"single shared owner submits menu PCM");music.request(gf::MusicCue::Credits);
    for(unsigned i=0;i<4;++i){music_sink.consume(2048);check(step(),"shared owner fade step");}
    check(music.current()==gf::MusicCue::Credits&&music.switches()==1&&music_sink.queued.front()[0]==0,"shared owner starts credits at floor without extra decoder admission");
    gf::PcmContributions invalid;std::vector<gf::s16> output;
    check(!gf::pcm_encode(invalid,0,1,&output),"invalid journal shape refuses");
    invalid.frames=1;invalid.bus_mask=1;for(auto& bus:invalid.bus)bus.resize(2);invalid.bus[0][0]=std::numeric_limits<gf::s64>::max();
    check(!gf::pcm_encode(invalid,0,1,&output),"forged wide contributions refuse before signed overflow");
    gf::PcmMixer late;
    check(late.begin(1)&&late.set_policy(true,{1,1,1})&&late.add(gf::AudioBus::Sfx,std::vector<gf::s16>{4000,0})&&late.finish(&output)&&output==std::vector<gf::s16>({2000,2000}),"legal policy change after begin but before first source updates captured mono policy");
    std::printf("{\"pcm_queue_checks\":%u,\"synthetic_sources_only\":true,\"all_passed\":true}\n",checks);
}
