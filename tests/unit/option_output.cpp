#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/option_output.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* what){++checks;if(!ok){std::fprintf(stderr,"option output: %s\n",what);std::abort();}}
struct Haptics:gf::IHaptics {
    struct Event{unsigned port;float low,high;};std::vector<Event> events;std::array<bool,4> present={true,true,true,true};
    bool supported(unsigned p)const override{return p<4&&present[p];}
    void rumble(unsigned p,float low,float high)override{events.push_back({p,low,high});}
};
struct Audio:gf::IAudioBackend {
    std::vector<gf::s16> samples;unsigned submissions=0;
    bool open(unsigned,unsigned)override{return true;}void close()override{}
    void submit(const gf::s16* p,std::size_t frames)override{++submissions;samples.assign(p,p+frames*2);}
    gf::u64 frames_consumed()const override{return samples.size()/2;}
};
}
int main(){
    gf::PcmMixer mixer;std::vector<gf::s16> result={99};const std::vector<gf::s16> stereo={10000,-10000,30000,-20000};
    check(mixer.set_policy(false,{1,0.5f,0})&&mixer.begin(2)&&mixer.add(gf::AudioBus::Sfx,stereo)&&mixer.finish(&result)&&
        result==std::vector<gf::s16>({5000,-5000,15000,-10000}),"per-category gain and stereo separation");
    check(!mixer.finish(&result),"mix block consumed once");
    check(mixer.begin(2)&&mixer.add(gf::AudioBus::Voice,stereo)&&mixer.finish(&result)&&result==std::vector<gf::s16>(4,0),"zero bus gain");
    check(mixer.set_policy(true,{1,1,1})&&mixer.begin(2)&&mixer.add(gf::AudioBus::Bgm,stereo)&&mixer.add(gf::AudioBus::Voice,stereo)&&mixer.finish(&result)&&
        result==std::vector<gf::s16>({0,0,10000,10000}),"mono averages before clipping, equal output channels");
    check(mixer.set_policy(false,{1,1,1})&&mixer.begin(2)&&mixer.add(gf::AudioBus::Bgm,stereo)&&mixer.add(gf::AudioBus::Voice,stereo)&&mixer.finish(&result)&&
        result==std::vector<gf::s16>({20000,-20000,32767,-32768}),"mixed saturation, not signed overflow");
    check(!mixer.set_policy(false,{NAN,1,1})&&!mixer.set_policy(false,{-1,1,1})&&!mixer.set_policy(false,{2,1,1}),"invalid policy refuses");
    check(mixer.begin(2)&&mixer.add(gf::AudioBus::Bgm,stereo)&&!mixer.set_policy(true,{0,0,0})&&mixer.finish(&result)&&result==stereo,"policy cannot split an in-progress mix");
    check(mixer.begin(2)&&!mixer.add(gf::AudioBus(99),stereo)&&!mixer.finish(&result),"invalid bus invalidates block");
    check(mixer.begin(1)&&!mixer.add(gf::AudioBus::Bgm,stereo)&&!mixer.finish(&result),"invalid sample count doesn't publish partial mix");
    check(mixer.begin(2)&&!mixer.begin(0)&&!mixer.finish(&result)&&!mixer.begin(4097),"invalid frame budget cannot replay stale work");
    check(mixer.begin(2),"bounded source block");for(unsigned i=0;i<32;++i)check(mixer.add(gf::AudioBus::Sfx,stereo),"source within budget");
    check(!mixer.add(gf::AudioBus::Sfx,stereo)&&!mixer.finish(&result),"source budget failure refuses whole block");
    Audio audio;check(mixer.begin(2)&&mixer.add(gf::AudioBus::Bgm,stereo)&&mixer.submit(audio)&&audio.samples==stereo,"processed PCM reaches swappable sink");
    check(!mixer.submit(audio)&&audio.submissions==1,"submitted block not replayed");
    std::vector<gf::u8> bytes(40);auto word=[&](unsigned at,gf::u32 v){gf::store_be32(bytes.data()+at,v);};
    word(0,0x47464f53);word(4,1);word(8,123);for(unsigned i=0;i<5;++i)word(12+i*4,gf::f32_to_bits(i/4.0f));
    word(32,0x800a4000);word(36,0xffff0000);gf::OptionOutputProfile profile;std::string error;
    auto load=[&](const auto& b){return gf::option_output_load({b.data(),b.size()},123,&profile,&error);};
    check(load(bytes)&&profile.preview_updates==10,"owned profile binds curve and bounded preview");
    for(unsigned at:{0u,4u,8u,12u,32u,34u,36u,38u}){auto bad=bytes;bad[at]^=0x7f;check(!load(bad)&&profile.preview_updates==0,"malformed profile not published");}
    auto bad=bytes;gf::store_be32(bad.data()+16,gf::f32_to_bits(-1));check(!load(bad),"negative gain refuses");
    check(load(bytes),"reload valid owned profile");
    Haptics backend;
    {
        gf::OptionOutput output(backend);check(output.configure(profile),"portable output config");gf::OptionSettings settings;settings.rumble_mask=15;
        const bool applied=output.apply(settings,{3,2,1,0},false);output.update();
        check(applied&&backend.events.empty(),"loaded ON settings make no startup rumble, even on the next update");
        settings.rumble_mask=0;output.apply(settings,{3,2,1,0},false);
        settings.volume={2,1,0};settings.mono=1;check(output.apply(settings,{3,2,1,0},true),"all categories and mono apply");
        check(output.pcm().begin(2)&&output.pcm().add(gf::AudioBus::Bgm,stereo)&&output.pcm().finish(&result)&&result==std::vector<gf::s16>({0,0,2500,2500}),"settings drive actual PCM math");
        settings.rumble_mask=1;check(output.apply(settings,{3,2,1,0},true),"logical ON enables original preview");
        for(unsigned i=0;i<10;++i)output.update();
        check(backend.events.size()==10&&backend.events.back().port==3&&backend.events.back().low==1,"preview routed through logical-to-device binding for ten updates");
        output.update();check(backend.events.size()==11&&backend.events.back().port==3&&backend.events.back().low==0,"preview terminates and stops");
        output.update();check(backend.events.size()==11,"completed request does not restart");
        settings.rumble_mask=0;output.apply(settings,{3,2,1,0},true);settings.rumble_mask=2;output.apply(settings,{3,2,1,0},true);output.update();
        check(backend.events.back().port==2&&backend.events.back().low==1,"second logical port maps independently");
        gf::option_restore_values(settings);output.apply(settings,{3,2,1,0},true);
        check(backend.events.back().port==2&&backend.events.back().low==0,"default restoration stops active motor immediately");
        const auto count=backend.events.size();settings.volume[0]=5;check(!output.apply(settings,{0,1,2,3},true)&&backend.events.size()==count,"invalid settings have no partial side effects");
        settings.volume[0]=4;check(!output.apply(settings,{9,1,2,3},true)&&backend.events.size()==count,"invalid device mapping refuses before effects");
        settings.rumble_mask=1;output.apply(settings,{0,1,2,3},true);output.update();
    }
    check(backend.events.back().port==0&&backend.events.back().low==0,"shutdown stops owned active requests");
    {
        Haptics b;gf::RumblePolicy p(b);check(!p.configure(0)&&!p.configure(119)&&p.configure(10),"rumble work budget");
        check(p.set_policy(1,{1,1,2,3})&&p.preview(0),"duplicate mapping accepts first logical owner");p.update();
        p.set_policy(3,{1,1,2,3});p.preview(1);p.update();const auto before=b.events.size();
        p.set_policy(2,{1,1,2,3});check(b.events.size()==before+1&&b.events.back().port==1&&b.events.back().low==1,"disabled logical alias doesn't stop another active owner");
        p.set_policy(2,{2,3,1,0});check(b.events.back().port==1&&b.events.back().low==0,"remap stops old device and cancels pending requests");
        check(!p.preview(0)&&!p.preview(4),"disabled/invalid preview refuses");
        b.present[3]=false;p.preview(1);const auto count=b.events.size();p.update();check(b.events.size()==count,"no invented hardware effect on unsupported device");
    }
    bool real=false;std::ifstream f("local/content/option-output.gfos",std::ios::binary);
    if(f){std::vector<gf::u8>b((std::istreambuf_iterator<char>(f)),{});check(gf::option_output_load({b.data(),b.size()},gf::load_be32(b.data()+8),&profile,&error)&&profile.preview_updates==10,"real native output content");
        check(std::fabs(profile.level_gain[0]-0.6f)<1e-6f&&profile.level_gain[4]==1,"source common curve: lowest level is not mute");real=true;}
    std::printf("{\"option_output_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");
}
