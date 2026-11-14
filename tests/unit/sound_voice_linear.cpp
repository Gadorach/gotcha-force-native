#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/sound_voice_linear.hpp"
#include "engine/gameplay/sound_voice_parameters.hpp"
#include "tests/unit/sound_voice_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"voice linear seed: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;sound_voice_fixture::Bank bank;SoundVoiceLibrary library;std::array<SoundVoiceBankInput,1> input={bank.input()};check(library.load(input,&error),"owned explicit bank");auto b=sound_voice_fixture::profile();SoundVoiceProfile profile;check(sound_voice_profile_load({b.data(),b.size()},&profile,&error),"source scalar profile");SoundVoiceSeed first,second;check(sound_voice_seed(profile,library.find(0),0,0,36,&first,&error)&&sound_voice_seed(profile,library.find(0),0,1,36,&second,&error),"overlapping source key regions");SoundVoicePitch p0,p1;u32 r0=0,r1=0;check(sound_voice_pitch(profile,first,8192,&p0,&error)&&sound_voice_pitch(profile,second,8192,&p1,&error)&&sound_voice_parameters_detail::src(profile,p0,&r0)&&sound_voice_parameters_detail::src(profile,p1,&r1)&&r0!=r1,"qualified root-key pitch reaches distinct source SRC ratios");
    DspLinearWindow a,c;check(sound_voice_open_linear_window(first,r0,&a,&error)&&sound_voice_open_linear_window(second,r1,&c,&error),"seeds bind shared compressed type1 windows");std::vector<s16> x,y;check(a.read_window(12,&x,&error)&&c.read_window(12,&y,&error)&&x!=y&&a.source_state().rendered!=c.source_state().rendered,"two actual source histories advance at separate pitched rates");
    check(!sound_voice_open_linear_window({},r0,&a,&error)&&!a.ready()&&!a.read_window(1,&x,&error)&&x.empty(),"missing seed invalidates previous sample binding and output");check(sound_voice_open_linear_window(first,r0,&a,&error),"independent fresh binding after refusal");check(!sound_voice_open_linear_window(first,0x40001,&a,&error)&&!a.ready(),"unqualified ratio invalidates old binding");
    auto bad=first;bad.dsp=65535;check(!sound_voice_open_linear_window(bad,r0,&a,&error)&&!a.ready(),"foreign DSP index does not reuse previous source");
    auto broken=std::make_shared<SoundVoiceBankResource>(*first.resource);broken->bank.dsp[0].offset=u32(broken->payload.size());bad=first;bad.resource=broken;check(!sound_voice_open_linear_window(bad,r0,&a,&error)&&!a.ready(),"bound payload span checked before linear cursor load");
    broken=std::make_shared<SoundVoiceBankResource>(*first.resource);store_be32(broken->bank.dsp[0].header.data()+20,0xffffffffu);bad.resource=broken;check(!sound_voice_open_linear_window(bad,r0,&a,&error)&&!a.ready(),"malformed literal address refused without normalization");
    check(!sound_voice_open_linear_window(first,r0,nullptr,&error),"required output pointer");check(sound_voice_open_linear_window(first,r0,&a,&error),"new valid owned sample cursor");first={};second={};library={};bank.header.clear();bank.payload.clear();check(a.read_window(12,&x,&error)&&!x.empty(),"cursor owns compressed input after seed/library/caller release");
    std::printf("{\"sound_voice_linear_seed_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
