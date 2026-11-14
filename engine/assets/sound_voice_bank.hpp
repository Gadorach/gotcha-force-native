#pragma once
// Owned immutable normal sound-bank resources for shared startup/UI/actor voices.
// Inputs are loose TSB/CHD/DPK bytes with explicit bank IDs, never runtime AFS/DOL.
#include <memory>
#include <span>
#include "engine/assets/sound_bank.hpp"
namespace gf {
struct SoundVoiceBankInput {u8 id=0;ByteView tones,header,payload;};
struct SoundVoiceBankResource {u8 id=0;SoundBank bank;std::vector<u8> payload;};
class SoundVoiceLibrary {
public:
    bool load(std::span<const SoundVoiceBankInput> inputs,std::string* error){
        resources_.clear();ready_=false;
        auto fail=[&](const char* why){if(error)*error=std::string("voice library: ")+why;return false;};
        if(inputs.empty()||inputs.size()>8)return fail("explicit native bank budget");
        std::vector<std::shared_ptr<const SoundVoiceBankResource>> next;u64 bytes=0;
        for(const auto& input:inputs){
            if(input.id>127)return fail("bank ID domain");
            for(const auto& prior:next)if(prior->id==input.id)return fail("duplicate bank identity");
            auto bank=std::make_shared<SoundVoiceBankResource>();bank->id=input.id;
            if(!sound_voice_bank_load(input.tones,input.header,&bank->bank,error))return false;
            if(!input.payload.data||input.payload.size!=bank->bank.payload_bytes||bytes+input.payload.size>(64u<<20))return fail("payload binding/native total byte budget");
            bytes+=input.payload.size;bank->payload.assign(input.payload.data,input.payload.data+input.payload.size);next.push_back(std::move(bank));
        }
        resources_=std::move(next);ready_=true;if(error)error->clear();return true;
    }
    std::shared_ptr<const SoundVoiceBankResource> find(u8 bank)const{if(ready_)for(const auto& r:resources_)if(r->id==bank)return r;return {};}
    bool ready()const{return ready_;}std::size_t banks()const{return resources_.size();}
private:
    std::vector<std::shared_ptr<const SoundVoiceBankResource>> resources_;bool ready_=false;
};
struct SoundVoiceProfile {f32 src_scale=0,src_rate=0,volume_full=0,volume_default=0,bend_up=0,bend_down=0;f64 pitch_base=0,octave_cents=0;bool loaded=false;};
inline bool sound_voice_profile_valid(const SoundVoiceProfile& p){return p.loaded&&f32_to_bits(p.src_scale)==0x47800000u&&f32_to_bits(p.src_rate)==0x46fa0000u&&f32_to_bits(p.volume_full)==0x42fe0000u&&f32_to_bits(p.volume_default)==0x437f0000u&&f32_to_bits(p.bend_up)==0x45fff800u&&f32_to_bits(p.bend_down)==0x46000000u&&p.pitch_base==2.&&p.octave_cents==1200.;}
inline bool sound_voice_profile_load(ByteView bytes,SoundVoiceProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice profile: ")+why;return false;};
    if(!out||!bytes.data||bytes.size!=56||bytes.be32(0)!=0x47465649u||bytes.be16(4)!=1||bytes.be16(6)!=56||bytes.be32(32)!=100||bytes.be32(36)!=32767)return fail("header/source scalar operands");
    SoundVoiceProfile p;p.src_scale=bytes.bef32(8);p.src_rate=bytes.bef32(12);p.volume_full=bytes.bef32(16);p.volume_default=bytes.bef32(20);p.bend_up=bytes.bef32(24);p.bend_down=bytes.bef32(28);p.pitch_base=std::bit_cast<f64>((u64(bytes.be32(40))<<32)|bytes.be32(44));p.octave_cents=std::bit_cast<f64>((u64(bytes.be32(48))<<32)|bytes.be32(52));p.loaded=true;if(!sound_voice_profile_valid(p))return fail("unsupported source scalar values");*out=p;if(error)error->clear();return true;
}
} // namespace gf
