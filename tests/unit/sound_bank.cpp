#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <tuple>
#include "engine/assets/sound_bank.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"sound bank: %s\n",why);std::abort();}}
void be(std::vector<gf::u8>& b,unsigned at,gf::u32 v){gf::store_be32(b.data()+at,v);}
void le(std::vector<gf::u8>& b,unsigned at,gf::u32 v){for(unsigned i=0;i<4;++i)b[at+i]=gf::u8(v>>(i*8));}
std::vector<gf::u8> read(const char* path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    std::vector<gf::u8> tsb(32),chd(224),dpk(8,0);std::memcpy(tsb.data(),"TSBD",4);le(tsb,4,32);le(tsb,8,256);le(tsb,12,1);tsb[19]=40;tsb[31]=255;
    // Head32 + Prog48 + Smpl28 + Dspi116. Counts are inclusive maxima.
    std::memcpy(chd.data(),"Head",4);be(chd,4,32);be(chd,8,1);be(chd,12,224);be(chd,16,8);be(chd,20,32);be(chd,24,80);be(chd,28,108);
    for(auto [at,tag,size]:{std::tuple{32,"Prog",48},std::tuple{80,"Smpl",28},std::tuple{108,"Dspi",116}}){std::memcpy(chd.data()+at,tag,4);be(chd,at+4,size);be(chd,at+12,0xffffffff);}
    be(chd,48,20);chd[52]=1;chd[62]=40;chd[63]=40; // program with one key region
    const unsigned hdr=128;be(chd,hdr,14);be(chd,hdr+4,16);be(chd,hdr+8,44100);be(chd,hdr+16,2);be(chd,hdr+20,15);be(chd,hdr+24,2);
    gf::SoundBank bank;std::string error;std::vector<gf::SoundLayerBinding> layers;
    auto load=[&](const auto& t,const auto& c){return gf::sound_bank_load({t.data(),t.size()},{c.data(),c.size()},&bank,&error);};
    check(load(tsb,chd)&&bank.tones.size()==1&&bank.programs.size()==1&&bank.samples.size()==1&&bank.dsp.size()==1,"owned bank parses inclusive counts and published chunks");
    check(gf::sound_bank_resolve(bank,0,&layers,&error)&&layers.size()==1&&layers[0].sample==0,"owned event/program/key/sample/DSP resolution");
    const auto owned_tsb=tsb,owned_chd=chd;
    for(unsigned at:{0u,4u,8u,12u}){auto t=tsb;t[at]^=0x80;check(!load(t,chd)&&bank.tones.empty(),"owned bad tone header/version/extent/count refuses");}
    for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,36u,40u,44u,48u,80u,84u,88u,92u,108u,112u,116u,120u,124u,128u}){
        auto c=chd;c[at]^=0x80;check(!load(tsb,c)&&bank.dsp.empty(),"owned malformed chunk/count/pointer/payload/metadata refuses atomically");}
    auto c=chd;c[62]=41;check(!load(tsb,c),"inverted owned key interval refuses");
    c=chd;c[73]=1;check(!load(tsb,c),"region sample index beyond inclusive maximum refuses");
    c=chd;c[107]=1;check(!load(tsb,c),"sample DSP index beyond inclusive maximum refuses");
    c=chd;c.pop_back();check(!load(tsb,c),"truncated chunk refuses");
    c=chd;c.resize(c.size()+16,255);be(c,12,c.size());check(!load(tsb,c),"unexplained trailing chunk data refuses");
    check(!gf::sound_bank_load({nullptr,32},{chd.data(),chd.size()},&bank,&error)&&
        !gf::sound_bank_load({tsb.data(),tsb.size()},{chd.data(),chd.size()},nullptr,&error),"null input/destination refuses");
    check(load(owned_tsb,owned_chd),"owned bank reload before source lifetime check");
    tsb.clear();chd.clear();gf::DspAudio decoded;
    check(gf::sound_bank_decode(bank,0,{dpk.data(),dpk.size()},&decoded,&error)&&decoded.samples.size()==14,"bank owns metadata independently of input-buffer lifetime");
    check(!gf::sound_bank_decode(bank,1,{dpk.data(),dpk.size()},&decoded,&error)&&decoded.samples.empty(),"unknown sample refuses atomically");
    check(!gf::sound_bank_decode(bank,0,{dpk.data(),7},&decoded,&error),"wrong DPK extent refuses");
    auto initial=bank;bank.tones[0][15]=0;check(!gf::sound_bank_resolve(bank,0,&layers,&error)&&layers.empty(),"cyclic tone chain refuses without prefix");
    bank=initial;bank.tones[0][12]=1;check(!gf::sound_bank_resolve(bank,0,&layers,&error),"randomized tone is not silently made deterministic");
    bank=initial;bank.tones[0][0]=1;check(!gf::sound_bank_resolve(bank,0,&layers,&error),"stop/control tone requires separate operations");
    bank=initial;bank.tones[0][2]=1;check(!gf::sound_bank_resolve(bank,0,&layers,&error),"missing program refuses");
    bank=initial;bank.tones[0][3]=41;check(!gf::sound_bank_resolve(bank,0,&layers,&error),"no key-region match refuses");
    bank=initial;bank.programs[0].regions.resize(17,bank.programs[0].regions[0]);check(!gf::sound_bank_resolve(bank,0,&layers,&error)&&layers.empty(),"native layer budget refuses without partial voices");
    bank=initial;bank.tones.push_back(bank.tones[0]);bank.tones[0][15]=1;
    check(gf::sound_bank_resolve(bank,0,&layers,&error)&&layers.size()==2&&layers[0].tone==0&&layers[1].tone==1,"linked default tones retain source order");
    auto real_tsb=read("local/recomp-evidence/native-menu-sfx-20261109/snd_com01.tsb"),real_chd=read("local/recomp-evidence/native-menu-sfx-20261109/snd_com01.chd");bool real=false;
    if(!real_tsb.empty()&&!real_chd.empty()){
        const bool loaded=load(real_tsb,real_chd);if(!loaded)std::fprintf(stderr,"actual common bank refused: %s\n",error.c_str());
        check(loaded&&bank.programs.size()==2&&bank.samples.size()==105&&bank.dsp.size()==105,"actual common-bank metadata and inclusive maxima");
        for(unsigned id:{119u,122u,123u,124u}){
            check(gf::sound_bank_resolve(bank,id,&layers,&error),"actual source UI event resolves");
            const unsigned sample=id==119?40:id==122?34:id==123?35:36;
            check(layers.size()==(id==124?2:1)&&layers[0].sample==sample,"actual UI sample identity / layered confirmation");
        }
        check(layers[1].sample==37,"second confirmation layer is distinct original sample");
        const auto& sample=bank.samples[37];check(sample.raw[5]==37&&bank.tones[124][3]==36,"second layer needs source pitch correction; not an identical-rate fake stereo duplicate");
        for(unsigned at:{0u,4u,8u,12u}){auto t=real_tsb;t[at]^=0x80;check(!load(t,real_chd)&&bank.tones.empty(),"bad tone header fails atomically");}
        for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,48u,0x610u,0xb10u}){auto c=real_chd;c[at]^=0x80;check(!load(real_tsb,c)&&bank.dsp.empty(),"bad CHD chunk/version/extent/program/DSP bounds fails atomically");}
        check(load(real_tsb,real_chd),"real bank reload after failures");real=true;
    }
    std::printf("{\"sound_bank_checks\":%u,\"real_present\":%s,\"host_SFX_playback_implemented\":false,\"all_passed\":true}\n",checks,real?"true":"false");
}
