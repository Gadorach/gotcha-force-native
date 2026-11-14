#pragma once
// Source-qualified TSB -> CHD program/key-region -> sample -> DSP resolution.
// Owns parsed metadata, not pointers into an archive. No event playback, voice
// allocation, ADSR, pitch conversion, randomization or shared-queue policy here.
#include <array>
#include <cstring>
#include <string>
#include <vector>
#include "engine/assets/afs.hpp"
#include "engine/assets/dsp_adpcm.hpp"
namespace gf {
struct SoundRegion {std::array<u8,14> raw{};u16 sample=0;};
struct SoundProgram {bool present=false;std::array<u8,8> raw{};std::vector<SoundRegion> regions;};
struct SoundSample {std::array<u8,12> raw{};u16 dsp=0;};
struct SoundDsp {u32 offset=0;std::array<u8,96> header{};DspAdpcmInfo info;};
struct SoundBank {
    u32 payload_bytes=0;
    std::vector<std::array<u8,16>> tones;
    std::vector<SoundProgram> programs;std::vector<SoundSample> samples;std::vector<SoundDsp> dsp;
};
namespace sound_bank_detail {
inline constexpr u32 kMaxRecords=4096,kMaxHeader=1u<<20,kMaxPayload=32u<<20;
inline bool chunk(ByteView file,u32 at,const char* tag,u32* size,u32* count){
    if(at>file.size||file.size-at<16||std::memcmp(file.data+at,tag,4))return false;
    *size=file.be32(at+4);const u32 last=file.be32(at+8);
    if(*size<16||*size>file.size-at||last>=kMaxRecords||file.be32(at+12)!=0xffffffffu)return false;
    *count=last+1; // source bounds compare against the inclusive maximum index
    return true;
}
}
namespace sound_bank_detail {
using HeaderReader=bool(*)(ByteView,DspAdpcmInfo*,std::string*);
inline bool load(ByteView tone,ByteView chd,SoundBank* out,std::string* error,HeaderReader header_reader){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("sound bank: ")+why;return false;};
    if(!out||!header_reader||!tone.data||!chd.data||tone.size<16||tone.size>sound_bank_detail::kMaxHeader||chd.size<32||chd.size>sound_bank_detail::kMaxHeader)
        return fail("header budgets/destination");
    const u32 tones=load_le32(tone.data+12);
    if(std::memcmp(tone.data,"TSBD",4)||load_le32(tone.data+4)!=tone.size||load_le32(tone.data+8)!=256||
       !tones||tones>128||tone.size!=16+std::size_t(tones)*16)return fail("tone signature/version/count/extent");
    SoundBank next;next.payload_bytes=chd.be32(16);
    if(std::memcmp(chd.data,"Head",4)||chd.be32(4)!=32||chd.be32(8)!=1||chd.be32(12)!=chd.size||
       !next.payload_bytes||next.payload_bytes>sound_bank_detail::kMaxPayload)return fail("CHD header/version/payload budget");
    const u32 prog_at=chd.be32(20),sample_at=chd.be32(24),dsp_at=chd.be32(28);u32 prog_size=0,sample_size=0,dsp_size=0,programs=0,samples=0,dsps=0;
    if(prog_at!=32||!sound_bank_detail::chunk(chd,prog_at,"Prog",&prog_size,&programs)||sample_at!=prog_at+prog_size||
       !sound_bank_detail::chunk(chd,sample_at,"Smpl",&sample_size,&samples)||dsp_at!=sample_at+sample_size||
       !sound_bank_detail::chunk(chd,dsp_at,"Dspi",&dsp_size,&dsps)||
       16+u64(programs)*4>prog_size||16+u64(samples)*12>sample_size||16+u64(dsps)*100>dsp_size)return fail("published chunk/count/extent mismatch");
    const std::size_t end=std::size_t(dsp_at)+dsp_size;
    if(end!=chd.size&&((end+15)&~std::size_t(15))!=chd.size)return fail("unexpected trailing CHD extent");
    for(auto at=end;at<chd.size;++at)if(chd.data[at]!=255)return fail("unexpected CHD alignment padding");
    next.tones.resize(tones);for(unsigned i=0;i<tones;++i)std::copy_n(tone.data+16+i*16,16,next.tones[i].begin());
    next.programs.resize(programs);
    for(u32 i=0;i<programs;++i){const u32 offset=chd.be32(prog_at+16+i*4);if(offset==0xffffffffu)continue;
        if(offset<16+u64(programs)*4||offset>prog_size||prog_size-offset<8)return fail("program table offset");
        auto& p=next.programs[i];p.present=true;const u32 at=prog_at+offset;std::copy_n(chd.data+at,8,p.raw.begin());
        const u32 n=p.raw[0];if(u64(offset)+8+u64(n)*14>prog_size)return fail("program key regions exceed chunk");
        // Distinct published programs may share an exact descriptor, but cannot
        // reinterpret the middle of another program as a new descriptor.
        for(u32 j=0;j<i;++j)if(next.programs[j].present){const u32 other=chd.be32(prog_at+16+j*4),other_size=8+u32(next.programs[j].regions.size())*14;
            if(other!=offset&&offset<other+other_size&&other<offset+8+n*14)return fail("overlapping program descriptors");}
        p.regions.resize(n);for(u32 j=0;j<n;++j){auto& r=p.regions[j];const u32 pos=at+8+j*14;std::copy_n(chd.data+pos,14,r.raw.begin());r.sample=chd.be16(pos+12);
            if(r.raw[2]>r.raw[3]||r.sample>=samples)return fail("invalid region key/sample binding");}
    }
    next.samples.resize(samples);for(u32 i=0;i<samples;++i){auto& s=next.samples[i];const u32 at=sample_at+16+i*12;std::copy_n(chd.data+at,12,s.raw.begin());s.dsp=chd.be16(at+10);
        if(s.dsp>=dsps)return fail("sample/DSP binding");}
    next.dsp.resize(dsps);for(u32 i=0;i<dsps;++i){auto& d=next.dsp[i];const u32 at=dsp_at+16+i*100;d.offset=chd.be32(at);std::copy_n(chd.data+at+4,96,d.header.begin());
        if(!header_reader({d.header.data(),d.header.size()},&d.info,error))return false;
        const u64 bytes=dsp_detail::payload_bytes(d.info.nibbles);if(d.offset>next.payload_bytes||bytes>next.payload_bytes-d.offset)return fail("DSP payload extent");}
    *out=std::move(next);return true;
}
} // namespace sound_bank_detail
inline bool sound_bank_load(ByteView tone,ByteView chd,SoundBank* out,std::string* error){return sound_bank_detail::load(tone,chd,out,error,dsp_adpcm_header);}
inline bool sound_voice_bank_load(ByteView tone,ByteView chd,SoundBank* out,std::string* error){return sound_bank_detail::load(tone,chd,out,error,dsp_voice_header);}
struct SoundLayerBinding {u32 tone=0,program=0,region=0,sample=0,dsp=0;};
inline bool sound_bank_resolve(const SoundBank& bank,u32 tone,std::vector<SoundLayerBinding>* out,std::string* error){
    if(out)out->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("sound bank resolve: ")+why;return false;};
    if(!out||bank.tones.size()>128)return fail("destination/tone budget");
    std::array<bool,128> seen{};std::vector<SoundLayerBinding> next;
    for(unsigned step=0;step<128;++step){
        if(tone>=bank.tones.size()||seen[tone])return fail("out-of-range/cyclic tone link");
        seen[tone]=true;const auto& t=bank.tones[tone];const u32 program=t[2];
        if(t[0]!=0||t[12]!=0||t[13]!=0)return fail("stop/randomized tone requires unported operations");
        if(program>=bank.programs.size()||!bank.programs[program].present)return fail("missing program");
        const auto& p=bank.programs[program];const auto before=next.size();
        for(u32 i=0;i<p.regions.size();++i){const auto& r=p.regions[i];if(r.raw[2]<=t[3]&&t[3]<=r.raw[3]){
            if(r.sample>=bank.samples.size()||bank.samples[r.sample].dsp>=bank.dsp.size()||next.size()>=16)return fail("layer binding/native budget");
            next.push_back({tone,program,i,r.sample,bank.samples[r.sample].dsp});}}
        if(next.size()==before)return fail("no matching key region");
        if(t[15]==255){*out=std::move(next);return true;}
        tone=t[15];
    }
    return fail("tone work budget");
}
inline bool sound_bank_decode(const SoundBank& bank,u32 sample,ByteView payload,DspAudio* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("sound bank decode: ")+why;return false;};
    if(!out||!payload.data||payload.size!=bank.payload_bytes||sample>=bank.samples.size()||bank.samples[sample].dsp>=bank.dsp.size())return fail("payload/sample binding");
    const auto& d=bank.dsp[bank.samples[sample].dsp];const auto bytes=dsp_detail::payload_bytes(d.info.nibbles);
    if(d.offset>payload.size||bytes>payload.size-d.offset)return fail("payload span");
    return dsp_adpcm_decode({d.header.data(),d.header.size()},{payload.data+d.offset,std::size_t(bytes)},out,error);
}
} // namespace gf
