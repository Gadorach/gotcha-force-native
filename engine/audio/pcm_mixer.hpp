#pragma once
// Portable native PCM bus. No device APIs, codecs, sound-ID inference or guest
// memory. Sources supply interleaved stereo at the sink's configured rate.
#include <array>
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>
#include "engine/core/backends.hpp"
namespace gf {
enum class AudioBus : unsigned { Bgm,Sfx,Voice };
// Already policy/envelope-weighted, UNCLIPPED bus contributions. Retained queued
// blocks can drop a bus and re-encode without decoding sources, applying gains a
// second time, losing cancellation/clipping information, or changing mono policy.
struct PcmContributions {
    unsigned frames=0,bus_mask=0;bool mono=false;
    std::array<std::vector<s64>,3> bus;
    // Explicit source-policy-weighted native wide SFX bus. Default false keeps
    // the historical32-source bound and ordinary gain path unchanged.
    bool source_weighted_sfx=false;
    //Opt-in breakdown of the already weighted Voice bus by physical player0/1.
    //Default empty leaves the ordinary aggregate Voice path unchanged. Inactive
    //lane data may remain after cuts; mask determines the aggregate, not storage.
    struct VoiceSlots {bool source_weighted=false;unsigned mask=0;std::array<std::vector<s64>,2> lane;} voice_slots;
};
inline constexpr s64 kPcmVoiceSlotLimit=s64(32768)*65536;
inline bool pcm_voice_slots_valid(const PcmContributions& block){
    const auto& v=block.voice_slots;if(!v.source_weighted)return v.mask==0&&v.lane[0].empty()&&v.lane[1].empty();
    const std::size_t size=std::size_t(block.frames)*2;if(!block.frames||block.frames>4096||v.mask>3||block.bus[2].size()!=size)return false;
    for(unsigned slot=0;slot<2;++slot){const auto& lane=v.lane[slot];if((v.mask&(1u<<slot))?lane.size()!=size:(!lane.empty()&&lane.size()!=size))return false;for(auto value:lane)if(value< -kPcmVoiceSlotLimit||value>kPcmVoiceSlotLimit)return false;}
    for(std::size_t i=0;i<size;++i){s64 sum=0;for(unsigned slot=0;slot<2;++slot)if(v.mask&(1u<<slot))sum+=v.lane[slot][i];if(block.bus[2][i]!=sum)return false;}return true;
}
//Pure filtering of validated weighted contributions. Does not reweight, decode,
//advance a source or restore a bus removed by a previous whole-bus cut.
inline bool pcm_drop_voice_slot(PcmContributions* block,unsigned slot){
    if(!block||slot>=2)return false;
    if(!(block->bus_mask&(1u<<unsigned(AudioBus::Voice))))return true;
    if(!block->voice_slots.source_weighted||!pcm_voice_slots_valid(*block))return false;
    auto& v=block->voice_slots;v.mask&=~(1u<<slot);for(std::size_t i=0;i<block->bus[2].size();++i){s64 sum=0;for(unsigned lane=0;lane<2;++lane)if(v.mask&(1u<<lane))sum+=v.lane[lane][i];block->bus[2][i]=sum;}if(!v.mask)block->bus_mask&=~(1u<<unsigned(AudioBus::Voice));return true;
}
inline bool pcm_encode(const PcmContributions& block,unsigned first,unsigned count,std::vector<s16>* out){
    if(!out||!block.frames||block.frames>4096||block.bus_mask>7||first>block.frames||!count||count>block.frames-first)return false;
    for(const auto& bus:block.bus)if(bus.size()!=std::size_t(block.frames)*2)return false;
    if(!pcm_voice_slots_valid(block))return false;
    std::vector<s16> result(std::size_t(count)*2);
    for(unsigned i=0;i<count;++i){s64 l=0,r=0;
        for(unsigned b=0;b<3;++b)if(block.bus_mask&(1u<<b)){
            const auto a=block.bus[b][std::size_t(first+i)*2],z=block.bus[b][std::size_t(first+i)*2+1];
            const s64 limit=block.source_weighted_sfx&&b==unsigned(AudioBus::Sfx)?s64(2147483648ll)*65536:s64(32)*32768*65536;
            if(a< -limit||a>limit||z< -limit||z>limit)return false;
            l+=a;r+=z;}
        const s64 left=block.mono?(l+r)/2:l,right=block.mono?left:r;
        result[i*2]=s16(std::clamp<s64>(left/65536,-32768,32767));result[i*2+1]=s16(std::clamp<s64>(right/65536,-32768,32767));
    }
    *out=std::move(result);return true;
}
class PcmMixer {
public:
    static constexpr unsigned kMaxFrames=4096,kMaxSources=32;
    bool mono_policy()const{return mono_;}
    bool set_policy(bool mono,const std::array<f32,3>& gains){
        if(ready_&&sources_)return false; // don't split a mix block across policies
        std::array<u32,3> next{};
        for(unsigned i=0;i<3;++i){if(!std::isfinite(gains[i])||gains[i]<0||gains[i]>1)return false;next[i]=u32(gains[i]*65536.0f+0.5f);}
        mono_=mono;gain_=next;if(ready_)block_.mono=mono;return true;
    }
    bool begin(unsigned frames){
        if(!frames||frames>kMaxFrames){ready_=false;return false;}
        block_.frames=frames;block_.mono=mono_;block_.bus_mask=0;block_.source_weighted_sfx=false;
        block_.voice_slots.source_weighted=false;block_.voice_slots.mask=0;for(auto& lane:block_.voice_slots.lane)lane.clear();
        for(auto& bus:block_.bus)bus.assign(std::size_t(frames)*2,0);
        frames_=frames;sources_=0;ready_=true;return true;
    }
    bool add(AudioBus bus,std::span<const s16> stereo,f32 envelope=1.0f){
        const unsigned category=unsigned(bus);
        if(!ready_||category>=3||(category==unsigned(AudioBus::Sfx)&&block_.source_weighted_sfx)||(category==unsigned(AudioBus::Voice)&&block_.voice_slots.source_weighted)||stereo.size()!=std::size_t(frames_)*2||sources_>=kMaxSources||
           !std::isfinite(envelope)||envelope<0||envelope>1){ready_=false;return false;}
        const u32 gain=u32(f32(gain_[category])*envelope+0.5f);
        ++sources_;block_.bus_mask|=1u<<category;
        for(std::size_t i=0;i<stereo.size();++i)block_.bus[category][i]+=s64(stereo[i])*gain;
        return true;
    }
    // Already source VE/pan/settings-weighted, UNCLIPPED Q16 stereo. Do not apply
    // host SFX preference again. Only one exclusive weighted SFX bus per block;
    // ordinary BGM/Voice still use set_policy/add. Final clip stays pcm_encode.
    bool add_source_weighted_sfx(std::span<const s64> stereo){
        if(!ready_||stereo.size()!=std::size_t(frames_)*2||sources_>=kMaxSources||(block_.bus_mask&(1u<<unsigned(AudioBus::Sfx)))){ready_=false;return false;}
        constexpr s64 limit=s64(2147483648ll)*65536;
        for(auto value:stereo)if(value< -limit||value>limit){ready_=false;return false;}
        block_.bus[unsigned(AudioBus::Sfx)].assign(stereo.begin(),stereo.end());block_.bus_mask|=1u<<unsigned(AudioBus::Sfx);block_.source_weighted_sfx=true;++sources_;return true;
    }
    //Explicit source-policy/envelope-weighted Q16 voice lane. The host Voice
    //preference is NOT applied again. Source SDK gain conversion/slot generation
    //ownership remain the upstream producer's responsibility, not this mixer.
    bool add_source_weighted_voice_slot(unsigned slot,std::span<const s64> stereo){
        if(!ready_||slot>=2||stereo.size()!=std::size_t(frames_)*2||sources_>=kMaxSources||((block_.bus_mask&(1u<<unsigned(AudioBus::Voice)))&&!block_.voice_slots.source_weighted)||(block_.voice_slots.mask&(1u<<slot))){ready_=false;return false;}
        for(auto value:stereo)if(value< -kPcmVoiceSlotLimit||value>kPcmVoiceSlotLimit){ready_=false;return false;}
        auto& voice=block_.voice_slots;voice.lane[slot].assign(stereo.begin(),stereo.end());voice.source_weighted=true;voice.mask|=1u<<slot;block_.bus_mask|=1u<<unsigned(AudioBus::Voice);for(std::size_t i=0;i<stereo.size();++i)block_.bus[2][i]+=stereo[i];++sources_;return true;
    }
    bool finish(std::vector<s16>* out,PcmContributions* contributions=nullptr){
        if(!ready_||!out)return false;
        if(!pcm_encode(block_,0,frames_,out)){ready_=false;return false;}
        if(contributions)*contributions=block_;
        ready_=false;return true;
    }
    // Caller opens a stereo sink at the source rate. This is a submission to
    // the seam, not confirmation of hardware playback; no resampler is implied.
    bool submit(IAudioBackend& sink){
        std::vector<s16> pcm;if(!finish(&pcm))return false;
        sink.submit(pcm.data(),frames_);return true;
    }
private:
    bool mono_=false,ready_=false;unsigned frames_=0,sources_=0;
    std::array<u32,3> gain_={65536,65536,65536};PcmContributions block_;
};
} // namespace gf
