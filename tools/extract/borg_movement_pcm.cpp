// Offline conversion of three qualified normal GG4E08 movement tones.
// Input files must be extracted by borg_movement_sfx.py. Stdout is raw PCM;
// this program never writes paths, changes source files, or supplies game timing.
#include <cstdio>
#include <fstream>
#include <string_view>
#include "engine/assets/sound_bank.hpp"

namespace {
using namespace gf;
struct Tone {std::string_view name;u32 tone,sample;bool loop;};
constexpr std::array tones{Tone{"landing",15,15,false},Tone{"jump",30,29,false},Tone{"thrust",40,27,true}};
bool read(const char* path,std::size_t size,std::vector<u8>* out){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file||file.tellg()!=std::streamoff(size))return false;
    out->resize(size);file.seekg(0);
    return bool(file.read(reinterpret_cast<char*>(out->data()),std::streamsize(size)));
}
ByteView bytes(const std::vector<u8>& v){return {v.data(),v.size()};}
}
int main(int argc,char** argv){
    if(argc!=5){std::fputs("usage: borg_movement_pcm TSB CHD DPK landing|jump|thrust\n",stderr);return 2;}
    const auto tone=std::find_if(tones.begin(),tones.end(),[&](auto id){return id.name==argv[4];});
    auto fail=[](const char* why,const std::string& error={}){
        std::fprintf(stderr,"Borg movement PCM: %s%s%s\n",why,error.empty()?"":": ",error.c_str());return 2;
    };
    if(tone==tones.end())return fail("unknown reviewed cue");
    std::vector<u8> tsb,chd,dpk;
    if(!read(argv[1],2064,&tsb)||!read(argv[2],13360,&chd)||!read(argv[3],3289888,&dpk))return fail("wrong input extents");
    SoundBank bank;DspAudio decoded;std::string error;std::vector<SoundLayerBinding> layers;
    if(!sound_bank_load(bytes(tsb),bytes(chd),&bank,&error)||!sound_bank_resolve(bank,tone->tone,&layers,&error)||
       layers.size()!=1||layers[0].sample!=tone->sample||
       !sound_bank_decode(bank,tone->sample,bytes(dpk),&decoded,&error))return fail("unqualified single-layer tone",error);
    const auto& info=decoded.info;
    if(info.rate!=44100||info.loop!=tone->loop)return fail("unexpected native sample rate or loop");
    // For a sustained package cue export the exact source loop interval only.
    // This does not reproduce the DSP voice's attack, gain envelope or release.
    const u32 start=info.loop?info.loop_start:0,end=info.loop?info.loop_end:info.frames;
    constexpr u32 output_rate=48000;
    if(end<=start||end>decoded.samples.size())return fail("sample extent");
    const u64 frames=(u64(end-start)*output_rate+info.rate-1)/info.rate;
    if(!frames||frames>output_rate*3)return fail("PCM frame budget");
    std::vector<u8> pcm(std::size_t(frames)*4);
    for(u64 i=0;i<frames;++i){
        const auto phase=i*info.rate;
        const u32 a=start+u32(phase/output_rate),b=a+1<end?a+1:info.loop?start:a;
        const u32 t=u32(phase%output_rate);
        const auto s=s16((s64(decoded.samples[a])*(output_rate-t)+s64(decoded.samples[b])*t)/output_rate);
        const auto bits=std::bit_cast<u16>(s);
        pcm[i*4]=pcm[i*4+2]=u8(bits);pcm[i*4+1]=pcm[i*4+3]=u8(bits>>8);
    }
    if(std::fwrite(pcm.data(),1,pcm.size(),stdout)!=pcm.size()||std::fflush(stdout)!=0)return fail("could not write PCM");
    return 0;
}
