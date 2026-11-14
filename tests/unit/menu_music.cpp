#include <cstdio>
#include <cstdlib>
#include "tests/unit/adx_fixture.hpp"
#include "engine/assets/menu_music.hpp"
#include "engine/gameplay/menu_music.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"menu music: %s\n",why);std::abort();}}
gf::AdxAudio pcm(gf::u32 frames=4096){gf::AdxAudio a;a.rate=48000;a.channels=2;a.frames=frames;a.loop=true;a.loop_start=32;a.loop_end=frames;
    for(unsigned i=0;i<frames;++i){a.samples.push_back(gf::s16(1000+i));a.samples.push_back(gf::s16(-int(i)));}return a;}
struct Sink:gf::IAudioBackend {
    bool present=true,reject_clear=false;unsigned calls=0,fail_at=0,legacy=0;gf::u64 queued=0,total=0,dropped=0;std::vector<gf::s16> last;
    bool open(unsigned,unsigned)override{return true;}void close()override{present=false;queued=0;}
    bool available()const override{return present;}
    void submit(const gf::s16*,std::size_t)override{++legacy;}
    bool try_submit(const gf::s16* p,std::size_t frames)override{++calls;if(calls==fail_at)return false;last.assign(p,p+frames*2);queued+=frames;total+=frames;return true;}
    bool discard_queued()override{if(reject_clear)return false;dropped+=queued;queued=0;return true;}
    gf::u64 frames_consumed()const override{return total-queued-dropped;}gf::u64 frames_available()const override{return queued;}gf::u64 frames_discarded()const override{return dropped;}
    void consume(gf::u64 n){queued-=std::min(n,queued);}
};
struct Files:gf::IFileSystem {
    std::vector<gf::u8> bytes;unsigned bulk=0,ranges=0;bool short_read=false;
    bool read(const std::string&,std::vector<gf::u8>&)override{++bulk;return false;}
    bool file_size(const std::string&,gf::u64* out)override{*out=bytes.size();return true;}
    bool read_range(const std::string&,gf::u64 at,std::size_t n,std::vector<gf::u8>& out)override{++ranges;if(at>bytes.size()||n>bytes.size()-at)return false;out.assign(bytes.begin()+at,bytes.begin()+at+n);if(short_read&&!out.empty())out.pop_back();return true;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}bool exists(const std::string&)override{return true;}bool erase(const std::string&)override{return false;}
};
void le(std::vector<gf::u8>& b,unsigned at,gf::u32 v){for(unsigned i=0;i<4;++i)b[at+i]=gf::u8(v>>(i*8));}
}
int main(){
    gf::AdxAudio mono;mono.rate=8000;mono.channels=1;mono.frames=3;mono.samples={0,1000,2000};gf::PcmPlayer player;std::vector<gf::s16> result;gf::u32 produced=0;
    check(player.load(mono,16000)&&player.play()&&player.render(8,&result,&produced)&&produced==6&&result==std::vector<gf::s16>({0,0,500,500,1000,1000,1500,1500,2000,2000,2000,2000,0,0,0,0}),"rational upsampling, mono duplication and bounded terminal zero tail");
    check(!player.playing()&&!player.render(1,&result),"EOF doesn't replay stale PCM");
    mono.frames=4;mono.samples={0,1000,2000,3000};mono.loop=true;mono.loop_start=1;mono.loop_end=3;
    check(player.load(mono,8000)&&player.play()&&player.render(8,&result)&&result==std::vector<gf::s16>({0,0,1000,1000,2000,2000,1000,1000,2000,2000,1000,1000,2000,2000,1000,1000})&&player.loops()==3,"intro then metadata loop, no trailing file samples");
    check(player.load(mono,16000)&&player.play()&&player.render(6,&result)&&result[10]==1500,"interpolation wraps to loop start, not into post-loop tail");
    auto a=pcm(128);a.rate=44100;a.loop_start=32;a.loop_end=120;gf::PcmPlayer whole,chunked;whole.load(a,48000);chunked.load(a,48000);whole.play();chunked.play();
    std::vector<gf::s16> expected,joined;whole.render(512,&expected);
    for(unsigned frames:{1u,7u,11u,33u,460u}){chunked.render(frames,&result);joined.insert(joined.end(),result.begin(),result.end());}
    check(joined==expected&&whole.phase()==chunked.phase()&&whole.loops()==chunked.loops()&&whole.source_position()==chunked.source_position(),"rate/loop phase independent of output block boundaries");
    check(!player.render(4097,&result)&&!player.load(mono,0),"player rate/work budgets");
    gf::PcmMixer mixer;Sink sink;gf::MenuMusic music;check(music.load(pcm(),48000)&&music.set_scope(true,sink)&&music.pump(mixer,sink),"native queue pump starts");
    check(sink.queued==2048&&sink.calls==2&&music.generated()==2048&&music.accepted()==2048&&sink.legacy==0,"queue watermark and checked path, not per-render-frame duration");
    check(music.pump(mixer,sink)&&sink.calls==2,"full queue doesn't advance source");sink.consume(2048);music.pump(mixer,sink);
    check(music.wraps()==1&&music.source_position()==32,"source wraps when generated PCM reaches loop endpoint");
    check(music.set_scope(false,sink)&&sink.queued==0&&sink.dropped==2048&&sink.frames_consumed()==2048,"credits/unsupported scope cuts queue without calling discarded frames played");
    mixer.set_policy(true,{0.5f,1,1});check(music.set_scope(true,sink)&&music.pump(mixer,sink)&&sink.last[0]==250&&sink.last[1]==250,"fresh menu scope uses current mono/gain policy");
    Sink absent;absent.present=false;gf::MenuMusic quiet;quiet.load(pcm(),48000);quiet.set_scope(true,absent);
    check(quiet.pump(mixer,absent)&&quiet.generated()==0&&absent.calls==0&&quiet.source_position()==0,"unavailable sink doesn't fake playback or advance source");
    Sink failed;failed.fail_at=2;gf::MenuMusic stopped;stopped.load(pcm(),48000);stopped.set_scope(true,failed);
    check(!stopped.pump(mixer,failed)&&stopped.failed()&&stopped.accepted()==1024&&failed.calls==2,"submission failure stops after effects, no fallback replay");
    failed.fail_at=0;check(!stopped.pump(mixer,failed)&&failed.calls==2,"failed producer never silently retries");
    sink.reject_clear=true;check(!music.set_scope(false,sink)&&music.failed(),"unsupported queue cut refuses instead of pretending silence");
    Files fs;const auto adx=owned_adx::file(64,2,true);fs.bytes.resize(64+adx.size());le(fs.bytes,0,gf::kAfsMagic);le(fs.bytes,4,1);le(fs.bytes,8,64);le(fs.bytes,12,gf::u32(adx.size()));std::copy(adx.begin(),adx.end(),fs.bytes.begin()+64);
    gf::MenuMusicProfile profile;profile.archive_count=1;profile.archive_size=fs.bytes.size();profile.directory_hash=gf::music_hash({fs.bytes.data(),16});profile.entry=0;profile.offset=64;profile.size=gf::u32(adx.size());profile.payload_hash=gf::music_hash({adx.data(),adx.size()});profile.source_rate=8000;profile.frames=64;profile.loop=true;profile.loop_start=0;profile.loop_end=64;profile.output_rate=48000;
    gf::AdxAudio decoded;std::string error;
    check(gf::menu_music_load(fs,"owned.afs",profile,&decoded,&error)&&decoded.frames==64&&fs.bulk==0&&fs.ranges==2,"bounded range-only AFS directory and selected payload");
    for(unsigned which=0;which<5;++which){auto bad=profile;if(which==0)bad.archive_size++;if(which==1)bad.directory_hash^=1;if(which==2)bad.offset++;if(which==3)bad.payload_hash^=1;if(which==4)bad.frames++;
        check(!gf::menu_music_load(fs,"owned.afs",bad,&decoded,&error)&&decoded.samples.empty(),"source/directory/payload/metadata identity refuses atomically");}
    fs.short_read=true;check(!gf::menu_music_load(fs,"owned.afs",profile,&decoded,&error),"backend short range doesn't become valid input");fs.short_read=false;
    std::vector<gf::u8> bytes(80);unsigned at=0;for(gf::u32 v:{0x47464d42u,1u,123u,1u,0u,gf::u32(fs.bytes.size()),profile.directory_hash,0u,64u,gf::u32(adx.size()),profile.payload_hash,8000u,64u,0u,64u,1u,48000u,0u,2u,0u}){gf::store_be32(bytes.data()+at,v);at+=4;}
    check(gf::menu_music_profile_load({bytes.data(),bytes.size()},123,&profile,&error),"owned native profile");
    for(unsigned i:{0u,4u,8u,12u,28u,36u,44u,48u,52u,56u,60u,64u,68u,72u,76u}){auto bad=bytes;bad[i]^=0x80;check(!gf::menu_music_profile_load({bad.data(),bad.size()},123,&profile,&error),"malformed native profile refuses");}
    std::printf("{\"menu_music_checks\":%u,\"all_passed\":true}\n",checks);
}
