#pragma once
// Bounded mono Nintendo DSP-ADPCM. Header and payload are separate: the bank
// stores standard 0x60-byte headers in CHD and compressed frames in DPK.
// Independent scalar decoder; no SDK/FFmpeg/emulator runtime dependency.
#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
inline constexpr u32 kMaxDspSamples=1u<<24;
struct DspAdpcmInfo {
    u32 frames=0,nibbles=0,rate=0,loop_start=0,loop_end=0;bool loop=false;
    std::array<s16,16> coefficients{};
    u16 initial_ps=0,loop_ps=0;s16 initial_h1=0,initial_h2=0,loop_h1=0,loop_h2=0;
};
struct DspAudio {DspAdpcmInfo info;std::vector<s16> samples;};
namespace dsp_detail {
inline s16 signed16(u16 v){return s16(v<32768?s32(v):s32(v)-65536);}
inline u32 sample_of(u32 nibble){return (nibble/16)*14+(nibble%16>=2?nibble%16-2:0);}
inline bool audio_nibble(u32 n){return n%16>=2;}
inline s64 floor2048(s64 v){return v>=0?v/2048:-((-v+2047)/2048);}
inline u64 payload_bytes(u32 nibbles){return ((u64(nibbles)+15)/16)*8;}
// Shared predictor step for declared-pass and incremental compressed playback.
// Callers validate the frame selector and pass a single high-first nibble.
inline s16 sample(unsigned bits,s64 scale,s32 c0,s32 c1,s32& h1,s32& h2){
    const s32 residual=bits<8?s32(bits):s32(bits)-16;
    const s64 sum=s64(residual)*scale*2048+s64(c0)*h1+s64(c1)*h2+1024;
    const auto value=s16(std::clamp<s64>(floor2048(sum),-32768,32767));h2=h1;h1=value;return value;
}
}
namespace dsp_detail {
inline bool header_impl(ByteView header,DspAdpcmInfo* out,std::string* error,bool source_addresses){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("dsp adpcm: ")+why;return false;};
    if(!out||!header.data||header.size!=96)return fail("header size/destination");
    DspAdpcmInfo next;next.frames=header.be32(0);next.nibbles=header.be32(4);next.rate=header.be32(8);
    const u32 flag=header.be16(12),start=header.be32(16),end=header.be32(20);
    if(!next.frames||next.frames>kMaxDspSamples||next.rate<8000||next.rate>192000||flag>1||header.be16(14)!=0||
       header.be32(24)!=2||header.be16(60)!=0)return fail("unsupported sample/rate/format/initial-offset/gain domain");
    const u64 expected=u64(next.frames/14)*16+(next.frames%14?2+next.frames%14:0);
    // Some source records include the two header nibbles of an empty terminal
    // frame when the declared sample count is exactly divisible by fourteen.
    const bool terminal_header=next.frames%14==0&&next.nibbles==expected+2;
    if(next.nibbles!=expected&&!terminal_header)return fail("incoherent sample/nibble/end bounds");
    const u64 encoded_nibbles=dsp_detail::payload_bytes(next.nibbles)*2;
    if(!dsp_detail::audio_nibble(end)||end>=(source_addresses?encoded_nibbles:u64(next.nibbles))||
       ((!source_addresses||flag!=0)&&(!dsp_detail::audio_nibble(start)||start>end)))return fail("incoherent sample/nibble/end bounds");
    next.loop=flag!=0;
    if(next.loop){next.loop_start=dsp_detail::sample_of(start);next.loop_end=dsp_detail::sample_of(end)+1;
        const u64 limit=source_addresses?(encoded_nibbles/16)*14:u64(next.frames);
        if(next.loop_start>=next.loop_end||next.loop_end>limit)return fail("loop interval");}
    for(unsigned i=0;i<16;++i)next.coefficients[i]=dsp_detail::signed16(header.be16(28+i*2));
    next.initial_ps=header.be16(62);next.initial_h1=dsp_detail::signed16(header.be16(64));next.initial_h2=dsp_detail::signed16(header.be16(66));
    next.loop_ps=header.be16(68);next.loop_h1=dsp_detail::signed16(header.be16(70));next.loop_h2=dsp_detail::signed16(header.be16(72));
    if(next.initial_ps>127||(next.loop&&next.loop_ps>127))return fail("unsupported predictor/scale context");
    *out=next;return true;
}
} // namespace dsp_detail
// Legacy raw decoder retains strict DECLARED-sample bounds, unchanged.
inline bool dsp_adpcm_header(ByteView header,DspAdpcmInfo* out,std::string* error){return dsp_detail::header_impl(header,out,error,false);}
// EDE6C's AX addresses are independent of declared PCM length. This separate
// entry admits only audio nibbles within the fully stored compressed frames;
// it never changes/clamps the original header or admits another payload frame.
inline bool dsp_voice_header(ByteView header,DspAdpcmInfo* out,std::string* error){return dsp_detail::header_impl(header,out,error,true);}
// One declared pass only. Retained loop context is metadata, not a claim that
// replaying already-decoded PCM reproduces the hardware's compressed-loop state.
inline bool dsp_adpcm_decode(ByteView header,ByteView payload,DspAudio* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("dsp adpcm: ")+why;return false;};
    if(!out)return fail("missing output");
    DspAudio next;if(!dsp_adpcm_header(header,&next.info,error))return false;
    const auto& info=next.info;const u64 bytes=dsp_detail::payload_bytes(info.nibbles);
    if(!payload.data||payload.size!=bytes||payload.data[0]!=info.initial_ps)return fail("payload size/initial predictor mismatch");
    if(info.loop){const u32 nibble=header.be32(16);if(payload.data[(nibble/16)*8]!=info.loop_ps)return fail("loop predictor mismatch");}
    next.samples.resize(info.frames);s32 h1=info.initial_h1,h2=info.initial_h2;
    for(u32 base=0;base<info.frames;base+=14){const auto at=std::size_t(base/14)*8;const unsigned predictor=payload.data[at]>>4;
        if(predictor>=8)return fail("invalid frame predictor");
        const s64 scale=s64(1)<<(payload.data[at]&15);const s32 c0=info.coefficients[predictor*2],c1=info.coefficients[predictor*2+1];
        for(unsigned i=0;i<14&&base+i<info.frames;++i){const u8 pair=payload.data[at+1+i/2];const unsigned bits=i%2?(pair&15):(pair>>4);
            next.samples[base+i]=dsp_detail::sample(bits,scale,c0,c1,h1,h2);
        }
    }
    *out=std::move(next);return true;
}
} // namespace gf
