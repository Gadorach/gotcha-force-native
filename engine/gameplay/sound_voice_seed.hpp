#pragma once
// EDE6C/EE11C/EE19C/EE614/EE6A4/EE71C: source voice initialization parameters.
// No backend allocation, envelope advancement, PCM mixing or playback ACK here.
#include <cmath>
#include "engine/assets/sound_voice_bank.hpp"
#include "engine/audio/dsp_stream.hpp"
namespace gf {
struct SoundVoiceSeed {
    std::shared_ptr<const SoundVoiceBankResource> resource;
    u16 program=0,region=0,sample=0,dsp=0;u8 priority=0,base_volume=0,base_pan=0;
    s32 base_cents=0;std::array<u16,2> channel_gain{},bend_cents{};u32 envelope_word=0,initial_src_q16=0;
    f32 source_rate=0;std::array<u16,18> initial_mix{};u16 initial_ve=0;u8 envelope_phase=0;
};
struct SoundVoicePitch {f32 cents=0,sample_rate=0;};
inline bool sound_voice_seed(const SoundVoiceProfile& profile,std::shared_ptr<const SoundVoiceBankResource> resource,u16 program,u16 region,u8 key,SoundVoiceSeed* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice seed: ")+why;return false;};
    if(!out||!sound_voice_profile_valid(profile)||!resource||program>=resource->bank.programs.size())return fail("profile/resource/program domain");
    const auto& p=resource->bank.programs[program];if(!p.present||p.raw[0]!=p.regions.size()||region>=p.regions.size())return fail("published program/region domain");const auto& r=p.regions[region];
    if(key<r.raw[2]||key>r.raw[3]||r.sample!=load_be16(r.raw.data()+12)||r.sample>=resource->bank.samples.size())return fail("matched region/sample identity");
    const auto& s=resource->bank.samples[r.sample];if(s.dsp!=load_be16(s.raw.data()+10)||s.dsp>=resource->bank.dsp.size())return fail("sample/DSP identity");const auto& d=resource->bank.dsp[s.dsp];DspAdpcmInfo info;if(!dsp_voice_header({d.header.data(),d.header.size()},&info,error))return false;
    const u64 bytes=dsp_detail::payload_bytes(info.nibbles);if(d.offset>resource->payload.size()||bytes>resource->payload.size()-d.offset)return fail("DSP payload span");
    SoundVoiceSeed next;next.resource=std::move(resource);next.program=program;next.region=region;next.sample=r.sample;next.dsp=s.dsp;next.priority=r.raw[0];
    auto volume=[&](u8 value){return value==u8(profile.volume_default)?profile.volume_full:f32(value);};
    const f32 a=volume(p.raw[2]),b=volume(r.raw[8]),c=volume(s.raw[6]);const f32 product=f32(f32(f32(f32(a/profile.volume_full)*b)/profile.volume_full)*c);
    if(!std::isfinite(product)||product<0||product>2147483647.f)return fail("derived volume domain");
    next.base_volume=u8(u32(product)); // source byte store wraps; no invented127 clamp
    const s32 relative=std::clamp(s32(p.raw[3])+s32(r.raw[9])-128,-64,63);next.base_pan=u8(std::clamp(relative+s32(s.raw[7]),0,127));
    auto signed_byte=[](u8 value){return s32(std::bit_cast<s8>(value));};
    next.base_cents=signed_byte(s.raw[8])*100+signed_byte(s.raw[9])+signed_byte(r.raw[10])*100+signed_byte(r.raw[11])+signed_byte(p.raw[4])*100+signed_byte(p.raw[5])+(s32(key)-s32(s.raw[5]))*100;
    for(unsigned side=0;side<2;++side){const unsigned shift=side?0:4;const u32 program_gain=(p.raw[1]>>shift)&15u,region_gain=(r.raw[1]>>shift)&15u;next.channel_gain[side]=u16((((program_gain*32767u)>>4)*region_gain)>>4);next.bend_cents[side]=load_be16(r.raw.data()+4+side*2);}
    next.envelope_word=load_be32(s.raw.data());next.source_rate=f32(info.rate);const f32 ratio=f32(profile.src_scale*f32(next.source_rate/profile.src_rate));
    if(!std::isfinite(ratio)||ratio<0||f64(ratio)>4294967295.)return fail("initial source ratio domain");
    next.initial_src_q16=u32(ratio);
    // EDE6C's phase08/mix6A..8D/VE8E reset is represented. Pitch cache, envelope
    // level and other unread source free-slot bytes are NOT claimed initialized.
    *out=std::move(next);if(error)error->clear();return true;
}
inline bool sound_voice_pitch(const SoundVoiceProfile& profile,const SoundVoiceSeed& seed,u16 pitch,SoundVoicePitch* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice pitch: ")+why;return false;};
    if(!out||!sound_voice_profile_valid(profile)||!seed.resource||!std::isfinite(seed.source_rate)||seed.source_rate<8000||seed.source_rate>192000||pitch>16383)return fail("source pitch/profile/sample-rate domain");
    const bool up=(pitch&0x2000u)!=0;const f32 factor=f32(f32(seed.bend_cents[up?1:0])/(up?profile.bend_up:profile.bend_down));SoundVoicePitch next;next.cents=std::fma(factor,f32(s32(pitch)-8192),f32(seed.base_cents));next.sample_rate=f32(f64(seed.source_rate)*std::pow(profile.pitch_base,f64(next.cents)/profile.octave_cents));
    if(!std::isfinite(next.cents)||!std::isfinite(next.sample_rate)||next.sample_rate<=0)return fail("derived rate domain");
    *out=next;if(error)error->clear();return true;
}
inline bool sound_voice_open_stream(const SoundVoiceSeed& seed,DspStream* stream,std::string* error){
    if(stream)*stream={}; // failed rebinding must not leave an older PCM cursor playable
    if(!stream||!seed.resource||seed.dsp>=seed.resource->bank.dsp.size()){if(error)*error="voice stream: seed/resource/DSP domain";return false;}
    const auto& d=seed.resource->bank.dsp[seed.dsp];DspAdpcmInfo info;
    if(!dsp_voice_header({d.header.data(),d.header.size()},&info,error))return false;
    const auto bytes=dsp_detail::payload_bytes(info.nibbles);if(d.offset>seed.resource->payload.size()||bytes>seed.resource->payload.size()-d.offset){if(error)*error="voice stream: payload bounds";return false;}
    return stream->load({d.header.data(),d.header.size()},{seed.resource->payload.data()+d.offset,std::size_t(bytes)},DspStreamMode::SourceAddresses,error);
}
} // namespace gf
