// Offline dry sample export. Reuses the shared bank, tone and DSP decoders.
// Writes one WAV to stdout; does not infer event timing, gain or layering.
#include <charconv>
#include <cstdio>
#include <fstream>
#include <string_view>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "engine/assets/sound_bank.hpp"

namespace {
using namespace gf;
bool read(const char* path,std::size_t cap,std::vector<u8>& out){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    const auto size=file.tellg();
    if(!file||size<=0||size>std::streamoff(cap))return false;
    out.resize(std::size_t(size));file.seekg(0);
    return bool(file.read(reinterpret_cast<char*>(out.data()),std::streamsize(out.size())));
}
ByteView view(const std::vector<u8>& data){return {data.data(),data.size()};}
void le(u8* target,u32 value,unsigned bytes){
    for(unsigned i=0;i<bytes;++i)target[i]=u8(value>>(8*i));
}
int fail(std::string_view error){std::fprintf(stderr,"retail Borg sound: %.*s\n",int(error.size()),error.data());return 2;}
}

int main(int argc,char** argv){
    if(argc!=5)return fail("usage: retail_borg_sound TSB CHD DPK TONE");
    const std::string_view text=argv[4];
    unsigned tone=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),tone);
    if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||tone>=128)return fail("invalid tone ID");
    std::vector<u8> tsb,chd,dpk;
    if(!read(argv[1],1u<<20,tsb)||!read(argv[2],1u<<20,chd)||!read(argv[3],32u<<20,dpk))return fail("input read or budget");
    std::string error;SoundBank bank;std::vector<SoundLayerBinding> layers;
    if(!sound_voice_bank_load(view(tsb),view(chd),&bank,&error)||
       !sound_bank_resolve(bank,tone,&layers,&error))return fail(error);
    if(layers.size()!=1)return fail("multilayer tone: dry export does not mix layers");
    constexpr u32 max_samples=192000u*30u;
    const auto& binding=layers.front();
    if(bank.dsp[binding.dsp].info.frames>max_samples)return fail("sample budget");
    DspAudio pcm;if(!sound_bank_decode(bank,binding.sample,view(dpk),&pcm,&error))return fail(error);
    const auto data_bytes=u32(pcm.samples.size())*2;
    std::array<u8,44> header{};
    std::memcpy(header.data(),"RIFF",4);le(header.data()+4,data_bytes+36,4);
    std::memcpy(header.data()+8,"WAVEfmt ",8);le(header.data()+16,16,4);
    le(header.data()+20,1,2);le(header.data()+22,1,2);
    le(header.data()+24,pcm.info.rate,4);le(header.data()+28,pcm.info.rate*2,4);
    le(header.data()+32,2,2);le(header.data()+34,16,2);
    std::memcpy(header.data()+36,"data",4);le(header.data()+40,data_bytes,4);
#ifdef _WIN32
    if(_setmode(_fileno(stdout),_O_BINARY)==-1)return fail("stdout binary mode");
#endif
    if(std::fwrite(header.data(),1,header.size(),stdout)!=header.size())return fail("WAV header write");
    std::array<u8,8192> block{};
    for(std::size_t at=0;at<pcm.samples.size();){
        const auto n=std::min(block.size()/2,pcm.samples.size()-at);
        for(std::size_t i=0;i<n;++i)le(block.data()+2*i,u16(pcm.samples[at+i]),2);
        if(std::fwrite(block.data(),2,n,stdout)!=n)return fail("WAV payload write");
        at+=n;
    }
    if(std::fflush(stdout)!=0)return fail("WAV flush");
    return 0;
}
