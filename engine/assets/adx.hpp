#pragma once
// Bounded unencrypted ADX v3, encoding3/18-byte blocks/4-bit residuals, mono or
// stereo. Independent implementation; no CRI/FFmpeg runtime or guest addresses.
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
inline constexpr u32 kMaxAdxFrames=1u<<24;
struct AdxAudio {
    u32 rate=0,channels=0,frames=0,loop_start=0,loop_end=0;bool loop=false;
    std::vector<s16> samples; // interleaved, exactly the declared sample count
};
namespace adx_detail {
inline s32 nearest_even(f32 value){
    const f64 low=std::floor(f64(value)),part=f64(value)-low;
    return s32(low+((part>0.5||(part==0.5&&(s32(low)&1)))?1:0));
}
inline s64 floor4096(s64 n){return n>=0?n/4096:-((-n+4095)/4096);}
}
inline bool adx_decode(ByteView file,AdxAudio* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("adx: ")+why;return false;};
    if(!out||!file.data||file.size<34||file.size>(32u<<20)||file.be16(0)!=0x8000)return fail("header/size budget");
    const u32 data=u32(file.be16(2))+4;
    if(data<34||data>4096||data>file.size||std::memcmp(file.data+data-6,"(c)CRI",6))return fail("invalid data offset/signature");
    if(file.data[4]!=3||file.data[5]!=18||file.data[6]!=4||file.data[18]!=3||file.data[19]!=0)return fail("unsupported encoding/version/encryption");
    AdxAudio next;next.channels=file.data[7];next.rate=file.be32(8);next.frames=file.be32(12);
    const u32 cutoff=file.be16(16),flag=file.be32(24);
    if((next.channels!=1&&next.channels!=2)||next.rate<8000||next.rate>192000||!next.frames||next.frames>kMaxAdxFrames||cutoff>next.rate/2||flag>1)
        return fail("unsupported channel/rate/sample/loop domain");
    const u64 groups=(u64(next.frames)+31)/32,group_bytes=next.channels*18;
    if(u64(data)+groups*group_bytes>file.size)return fail("truncated channel blocks");
    next.loop=flag!=0;
    if(next.loop){
        if(data<50)return fail("truncated loop header");
        next.loop_start=file.be32(28);next.loop_end=file.be32(36);
        if(next.loop_start>=next.loop_end||next.loop_end>next.frames||next.loop_start%32||
           file.be32(32)!=u64(data)+u64(next.loop_start/32)*group_bytes||
           file.be32(40)!=u64(data)+((u64(next.loop_end)+31)/32)*group_bytes)return fail("incoherent loop sample/byte bounds");
    }
    const f64 root=std::sqrt(2.0),a=root-std::cos(6.2831853071795864769*cutoff/next.rate),b=root-1;
    const f64 c=(a-std::sqrt((a+b)*(a-b)))/b;
    const s32 c0=adx_detail::nearest_even(f32(c*8192)),c1=adx_detail::nearest_even(f32(-c*c*4096));
    if(c0<0||c0>8192||c1>0||c1< -4096)return fail("invalid predictor coefficients");
    next.samples.resize(std::size_t(next.frames)*next.channels);s32 h1[2]={},h2[2]={};
    for(u64 group=0;group<groups;++group)for(unsigned channel=0;channel<next.channels;++channel){
        const auto at=std::size_t(u64(data)+group*group_bytes+channel*18);const u32 scale=file.be16(at);
        if(scale&0x8000)return fail("early EOF/encrypted block");
        for(unsigned i=0;i<32;++i){const u8 byte=file.data[at+2+i/2];const unsigned bits=i%2?(byte&15):(byte>>4);
            const s32 residual=bits>=8?s32(bits)-16:s32(bits);
            const s64 predicted=adx_detail::floor4096(s64(c0)*h1[channel]+s64(c1)*h2[channel]);
            const s32 sample=s32(std::clamp<s64>(predicted+s64(residual)*scale,-32768,32767));
            h2[channel]=h1[channel];h1[channel]=sample;
            const u64 frame=group*32+i;if(frame<next.frames)next.samples[std::size_t(frame)*next.channels+channel]=s16(sample);
        }
    }
    *out=std::move(next);return true;
}
} // namespace gf
