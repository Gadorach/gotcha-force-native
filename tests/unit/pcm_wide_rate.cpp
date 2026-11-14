#include <cstdio>
#include <cstdlib>
#include "engine/audio/pcm_wide_rate.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"wide rate: %s\n",why);std::abort();}}gf::s32 value(gf::u64 i,unsigned ch){return gf::s32((i*7919+ch*65537)%4000001)-2000000;}}
int main(){
    using namespace gf;std::string error;
    for(u32 target:{8000u,11025u,22050u,32000u,44100u,48000u,96000u,192000u}){PcmWideRate whole,parts;check(whole.load(target)&&parts.load(target),"qualified native output rate");u64 input=0;auto pull=[&](std::array<s32,2>* out,std::string*){*out={value(input,0),value(input,1)};++input;return true;};std::vector<s64> expected;check(whole.render(4096,pull,&expected,&error),"wide rational block");
        for(unsigned i=0;i<4096;++i){const u64 phase=u64(i)*32000;const auto at=phase/target,frac=phase%target;for(unsigned channel=0;channel<2;++channel){const s64 n=s64(value(at,channel))*(target-frac)+s64(value(at+1,channel))*frac;const s64 actual=(n/target)*65536+(n%target)*65536/target;check(expected[i*2+channel]==actual,"independent absolute-position rational weighted result without clipped samples");}}
        u64 partial_input=0;auto pp=[&](std::array<s32,2>* out,std::string*){*out={value(partial_input,0),value(partial_input,1)};++partial_input;return true;};std::vector<s64> fragmented;unsigned done=0;while(done<4096){std::vector<s64> block;const unsigned n=std::min(4096-done,1+done%113);check(parts.render(n,pp,&block,&error),"variable output partition");fragmented.insert(fragmented.end(),block.begin(),block.end());done+=n;}
        check(expected==fragmented&&whole.state()==parts.state()&&input==partial_input&&input==(u64(4096)*32000/target)+2,"output and lookahead phase/count independent of queue partition");
    }
    PcmWideRate rate;check(!rate.load(7999)&&!rate.load(192001)&&rate.load(48000),"invalid load resets binding");unsigned calls=0;auto failed=[&](std::array<s32,2>* out,std::string*){*out={1,2};return ++calls!=4;};std::vector<s64> out;const auto before=rate.state();check(!rate.render(64,failed,&out,&error)&&out.empty()&&rate.state()==before,"late pull failure does not mutate converter; caller must stage producer as well");
    auto full=[](std::array<s32,2>* out,std::string*){*out={std::numeric_limits<s32>::max(),std::numeric_limits<s32>::min()};return true;};check(rate.render(4096,full,&out,&error)&&out[0]==s64(std::numeric_limits<s32>::max())*65536&&out[1]==s64(std::numeric_limits<s32>::min())*65536,"full signed32 samples avoid numerator-times-Q16 overflow");check(!rate.render(4097,full,&out,&error)&&out.empty(),"bounded output block");
    std::printf("{\"wide_rate_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
