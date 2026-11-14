#include <cstdio>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/credits_music.hpp"
#include "engine/gameplay/credits_music.hpp"
#include "engine/gameplay/option_menu.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"credits music: %s\n",why);std::abort();}}
gf::AdxAudio clip(gf::s16 amplitude){gf::AdxAudio a;a.rate=48000;a.channels=2;a.frames=64;a.loop=true;a.loop_end=64;a.samples.assign(128,amplitude);return a;}
struct Sink:gf::IAudioBackend {
    bool present=true,reject_clear=false;unsigned calls=0,clears=0,fail_at=0;gf::u64 queued=0;std::vector<gf::s16> last;
    bool open(unsigned,unsigned)override{return true;}void close()override{}bool available()const override{return present;}
    void submit(const gf::s16*,std::size_t)override{}
    bool try_submit(const gf::s16* p,std::size_t n)override{++calls;if(calls==fail_at)return false;last.assign(p,p+n*2);queued+=n;return true;}
    bool discard_queued()override{++clears;if(reject_clear)return false;queued=0;return true;}
    gf::u64 frames_available()const override{return queued;}gf::u64 frames_consumed()const override{return 0;}
};
struct Save:gf::ISaveBackend {
    bool load(gf::u32,std::vector<gf::u8>&)override{return false;}bool store(gf::u32,const gf::u8*,std::size_t)override{return true;}bool erase(gf::u32)override{return false;}
};
struct Files:gf::IFileSystem {
    bool read(const std::string&,std::vector<gf::u8>&)override{return false;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool exists(const std::string&)override{return false;}bool erase(const std::string&)override{return false;}
    bool file_size(const std::string& path,gf::u64* out)override{std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)return false;*out=gf::u64(f.tellg());return true;}
    bool read_range(const std::string& path,gf::u64 at,std::size_t n,std::vector<gf::u8>& out)override{
        std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f||at>gf::u64(f.tellg())||n>gf::u64(f.tellg())-at)return false;
        f.seekg(std::streamoff(at));out.resize(n);f.read(reinterpret_cast<char*>(out.data()),n);return bool(f);
    }
};
std::vector<gf::u8> read(const char* path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    gf::MusicFade fade;gf::MusicCue cue;
    check(fade.configure({4,8,1},1)&&fade.request(gf::MusicCue::Credits),"owned scalar request");
    for(unsigned i=0;i<3;++i)check(fade.update(&cue)&&cue==gf::MusicCue::None&&fade.value()==6-i*2,"source descending scalar steps");
    check(fade.update(&cue)&&cue==gf::MusicCue::Credits&&fade.value()==0&&fade.slope()==2,"cue emitted only at zero, slope reversed");
    for(unsigned i=0;i<4;++i)check(fade.update(&cue)&&cue==gf::MusicCue::None,"source ascending scalar steps");
    check(fade.value()==8&&fade.slope()==0&&fade.envelope()==1,"upper boundary stops ramp");
    fade.reset(1);fade.request(gf::MusicCue::Credits);fade.update(&cue);fade.request(gf::MusicCue::Menu);
    check(fade.pending()==gf::MusicCue::Menu&&fade.slope()==-1.5f,"quick B replaces pending cue and recomputes slope from current value");
    for(unsigned i=0;i<4;++i)fade.update(&cue);
    check(cue==gf::MusicCue::Menu&&fade.slope()==1.5f&&fade.value()==0,"same-current cue may be explicitly restarted at floor");
    unsigned ramp_up=0;while(fade.slope()!=0&&ramp_up<100){fade.update(&cue);++ramp_up;}
    check(ramp_up==6&&fade.value()==8,"interrupted fade-in is not fabricated as a fixed requested duration");
    fade.reset(0.5f);fade.set_level(1,true);check(fade.value()==4&&fade.maximum()==4&&fade.envelope()==0.5f,"immediate control retains source upper clamp until new cue");
    fade.request(gf::MusicCue::Credits);fade.set_level(1,true);
    check(fade.pending()==gf::MusicCue::None&&fade.slope()==0&&fade.value()==4,"same-level immediate refresh cancels pending fade/cue");
    fade.reset(0);fade.request(gf::MusicCue::Credits);check(fade.slope()==-1&&fade.update(&cue)&&cue==gf::MusicCue::Credits&&fade.envelope()==0,"zero-current fallback step; no divide by zero");
    check(fade.update(&cue)&&fade.slope()==0,"zero-gain upper boundary terminates");
    check(!fade.reset(NAN)&&!fade.set_level(-1)&&!fade.request(gf::MusicCue::None)&&!fade.configure({0,8,1},1),"invalid controls refuse");
    for(unsigned level=0;level<5;++level){const float gains[]={0.6f,0.7f,0.8f,0.9f,1};const unsigned expected[]={33,33,32,32,32};
        fade.configure({32,127,1},gains[level]);fade.request(gf::MusicCue::Credits);unsigned down=0,back=0;
        do{check(fade.update(&cue),"real-curve scalar step bounded");++down;}while(cue==gf::MusicCue::None&&down<100);
        while(fade.slope()!=0&&back<100){fade.update(&cue);++back;}
        check(down==expected[level]&&back==expected[level]&&fade.envelope()==1,"five source curve fade counts match independent float32 replay");
    }
    fade.configure({4,8,1},1);
    for(unsigned i=0;i<20;++i){fade.request(i%2?gf::MusicCue::Menu:gf::MusicCue::Credits);for(unsigned j=0;j<3;++j)fade.update(&cue);}
    fade.request(gf::MusicCue::Menu);for(unsigned i=0;i<4;++i)fade.update(&cue);unsigned work=0;
    while(!fade.failed()&&work++<9000)fade.update(&cue);
    check(fade.failed(),"pathologically tiny/nonprogressing source slopes hit native safety budget, not forged completion");
    gf::CreditsMusic bank;gf::PcmMixer mixer;Sink sink;
    check(bank.load(clip(8000),clip(12000),48000,{4,8,1},1)&&bank.set_scope(true,sink)&&bank.pump(mixer,sink)&&sink.last[0]==8000,"menu source startup at full envelope");
    bank.request(gf::MusicCue::Credits);
    for(unsigned i=0;i<4;++i){sink.queued=0;check(bank.request(gf::MusicCue::Credits)&&bank.pump(mixer,sink),"duplicate desired state doesn't restart fade");}
    check(bank.current()==gf::MusicCue::Credits&&bank.switches()==1&&sink.last[0]==0&&sink.clears==1,"old queue cut and original credits source starts at zero envelope");
    for(unsigned i=0;i<4;++i){sink.queued=0;bank.pump(mixer,sink);}
    check(sink.last[0]==12000&&bank.fade().envelope()==1&&bank.cue_accepted(gf::MusicCue::Credits)>0,"credits PCM rises through same mixer without double volume gain");
    bank.request(gf::MusicCue::Menu);sink.queued=0;bank.pump(mixer,sink);bank.set_level(0.5f,true);
    check(bank.fade().pending()==gf::MusicCue::None&&bank.current()==gf::MusicCue::Credits,"BGM immediate control cancels pending menu request as source does");
    sink.queued=0;bank.request(gf::MusicCue::Menu);bank.pump(mixer,sink);
    check(bank.current()==gf::MusicCue::Credits,"cancelled pending request isn't secretly reissued every frame");
    check(bank.set_scope(false,sink)&&bank.set_scope(true,sink)&&bank.current()==gf::MusicCue::Menu&&bank.fade().envelope()==1,"unsupported-scope return remains explicit native menu restart policy");
    Sink absent;absent.present=false;gf::CreditsMusic quiet;quiet.load(clip(1),clip(2),48000,{4,8,1},1);quiet.set_scope(true,absent);quiet.request(gf::MusicCue::Credits);quiet.pump(mixer,absent);
    check(quiet.generated()==0&&quiet.switches()==0&&quiet.fade().value()==8,"no device doesn't fake source/fade progression");
    Sink bad;gf::CreditsMusic refused;refused.load(clip(1),clip(2),48000,{4,8,1},1);refused.set_scope(true,bad);refused.pump(mixer,bad);refused.request(gf::MusicCue::Credits);bad.reject_clear=true;
    bool ok=true;for(unsigned i=0;i<4&&ok;++i){bad.queued=0;ok=refused.pump(mixer,bad);}const auto calls=bad.calls;
    check(!ok&&refused.failed()&&!refused.pump(mixer,bad)&&bad.calls==calls,"failed cue switch/queue discard never replays or falls back");
    Sink submit_bad;gf::CreditsMusic submit_refused;
    submit_refused.load(clip(1),clip(2),48000,{1,8,1},1);submit_refused.set_scope(true,submit_bad);submit_refused.request(gf::MusicCue::Credits);submit_bad.fail_at=1;
    check(!submit_refused.pump(mixer,submit_bad)&&submit_refused.failed()&&submit_refused.current()==gf::MusicCue::Credits&&
        submit_refused.generated()>0&&submit_refused.accepted()==0,"failed first new-cue block is generated but never counted accepted");
    check(!submit_refused.pump(mixer,submit_bad)&&submit_bad.calls==1&&!submit_refused.set_scope(true,submit_bad),"failed submission latches; no retry or scope-based replay");
    gf::CreditsMusic fast;Sink fast_sink;fast.load(clip(1),clip(2),48000,{4,8,1},1);fast.set_scope(true,fast_sink);
    fast.request(gf::MusicCue::Credits);fast.pump(mixer,fast_sink);fast.request(gf::MusicCue::Menu);
    for(unsigned i=0;i<4;++i){fast_sink.queued=0;check(fast.pump(mixer,fast_sink),"quick cancellation pump");}
    check(fast.current()==gf::MusicCue::Menu&&fast.switches()==1&&fast.cue_accepted(gf::MusicCue::Credits)==0,"quick cancellation restarts menu at floor without admitting credits PCM");
    gf::PcmMixer envelopes;std::vector<gf::s16> samples;const std::vector<gf::s16> input={10000,-10000};
    check(envelopes.set_policy(false,{0.5f,1,1})&&envelopes.begin(1)&&envelopes.add(gf::AudioBus::Bgm,input,0.5f)&&envelopes.finish(&samples)&&samples==std::vector<gf::s16>({2500,-2500}),"envelope combines with bus policy once");
    check(envelopes.begin(1)&&!envelopes.add(gf::AudioBus::Bgm,input,NAN)&&!envelopes.finish(&samples),"invalid envelope invalidates block");
    Save backend;gf::OptionSettingsStore store(backend);gf::OptionMenu menu;menu.configure(35,30);std::array<gf::GamePadState,4> pads{};menu.begin(store,pads);
    gf::GamePadState up,down,left,a;up.dpad_up=true;down.dpad_down=true;left.dpad_left=true;a.a=true;
    auto tap=[&](gf::GamePadState pad){pads={};pads[0]=pad;auto event=menu.update(store,pads);pads={};menu.update(store,pads);return event;};
    tap(up);check(menu.row()==9&&tap(a).bgm_immediate,"restore refreshes BGM even if values are already defaults");
    tap(down);tap(down);check(menu.row()==1&&tap(left).bgm_immediate,"actual BGM edit requests immediate source control");
    tap(down);check(menu.row()==2&&!tap(left).bgm_immediate,"SFX edit does not cancel BGM fades");
    tap(up);tap(up);check(menu.row()==0&&!tap(left).bgm_immediate,"sound mode edit does not cancel BGM fades");
    // Owned format fixtures exercise refusal gates even without private assets.
    std::vector<gf::u8> owned_menu(80),owned_transition(112);
    auto word=[](auto& b,unsigned at,gf::u32 value){gf::store_be32(b.data()+at,value);};
    const gf::u32 words[]={0x47464d42,1,123,2,0,1000,456,0,128,64,789,44100,32,0,32,1,48000,0,2,0};
    for(unsigned i=0;i<20;++i)word(owned_menu,i*4,words[i]);
    word(owned_transition,0,0x47464d54);word(owned_transition,4,1);word(owned_transition,8,gf::music_hash({owned_menu.data(),owned_menu.size()}));
    word(owned_transition,12,32);word(owned_transition,16,std::bit_cast<gf::u32>(127.0f));word(owned_transition,20,std::bit_cast<gf::u32>(1.0f));
    std::copy(owned_menu.begin(),owned_menu.end(),owned_transition.begin()+32);word(owned_transition,32+28,1);word(owned_transition,32+32,256);
    gf::CreditsMusicProfile owned_profile;std::string owned_error;
    auto owned_load=[&](const auto& bytes){return gf::credits_music_profile_load({bytes.data(),bytes.size()},{owned_menu.data(),owned_menu.size()},123,&owned_profile,&owned_error);};
    check(owned_load(owned_transition)&&owned_profile.credits.entry==1,"owned combined profile validates without original data");
    for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,32u+8u,32u+12u,32u+20u,32u+24u,32u+64u,32u+72u}){
        auto bytes=owned_transition;bytes[at]^=0x80;
        check(!owned_load(bytes)&&owned_profile.credits.frames==0,"owned stale/malformed/incompatible binding fails atomically");
    }
    auto duplicate=owned_transition;word(duplicate,32+28,0);check(!owned_load(duplicate),"same menu/credits member refuses");
    auto zero_step=owned_transition;word(zero_step,20,0);check(!owned_load(zero_step),"zero fallback step refuses");
    auto nan_scalar=owned_transition;word(nan_scalar,16,std::bit_cast<gf::u32>(NAN));check(!owned_load(nan_scalar),"nonfinite full scale refuses");
    check(!gf::credits_music_profile_load({owned_transition.data(),111},{owned_menu.data(),80},123,&owned_profile,&owned_error)&&
        !gf::credits_music_profile_load({nullptr,112},{owned_menu.data(),80},123,&owned_profile,&owned_error)&&
        !gf::credits_music_profile_load({owned_transition.data(),112},{nullptr,80},123,&owned_profile,&owned_error)&&
        !gf::credits_music_profile_load({owned_transition.data(),112},{owned_menu.data(),80},123,nullptr,&owned_error),"short/null inputs and destination refuse");
    const auto menu_file=read("local/content/main-menu.gfmb"),profile_file=read("local/content/credits-music.gfmt");bool real=false;
    if(!menu_file.empty()&&!profile_file.empty()){
        gf::CreditsMusicProfile profile;std::string error;const auto scene=gf::load_be32(menu_file.data()+8);
        auto load=[&](const auto& bytes){return gf::credits_music_profile_load({bytes.data(),bytes.size()},{menu_file.data(),menu_file.size()},scene,&profile,&error);};
        check(load(profile_file)&&profile.credits.entry==27&&profile.fade.updates==32&&profile.fade.full_scale==127&&profile.fade.min_step==1,"real source cue/fade profile binds parent menu profile");
        for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,40u,44u}){auto bytes=profile_file;bytes[at]^=0x80;check(!load(bytes),"malformed/stale combined profile refuses");}
        check(load(profile_file),"real transition profile reload");Files files;gf::AdxAudio decoded;
        check(gf::menu_music_load(files,"original/extracted/files/poq_adx_usa.afs",profile.credits,&decoded,&error)&&decoded.frames==6701082&&decoded.loop_start==448&&decoded.loop_end==6653289,"actual source credits member range-decodes with metadata");
        auto reference=read("local/recomp-evidence/native-credits-music-20261109/ffmpeg.pcm");
        if(!reference.empty()){check(reference.size()>=decoded.samples.size()*2,"independent credits reference frame count");
            for(std::size_t i=0;i<decoded.samples.size();++i){const auto value=gf::u16(decoded.samples[i]);if(reference[i*2]!=gf::u8(value)||reference[i*2+1]!=gf::u8(value>>8))check(false,"credits reference PCM mismatch");}
            check(true,"all declared credits PCM samples equal independent decoder");}
        real=true;
    }
    std::printf("{\"credits_music_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");
}
