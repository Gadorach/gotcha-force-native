#pragma once
// Explicit GC AX type1, nonstreaming, unfiltered 32-sample voice model.
// End latches during this subframe: remaining SRC input requests return zero,
// this subframe still mixes, and subsequent stopped subframes do not advance.
// This is native PCM/state, NOT allocation, aux effects, dpop application or ACK.
#include "engine/audio/dsp_linear_kernel.hpp"
#include "engine/audio/dsp_stream.hpp"
#include "engine/audio/pcm_q15.hpp"
namespace gf {
enum class DspVoiceLane : unsigned {MainLeft,MainRight,AuxALeft,AuxARight,AuxBLeft,AuxBRight,AuxBSurround,MainSurround,AuxASurround};
inline constexpr unsigned kDspVoiceSubframeSamples=32;
struct DspVoiceMixState {
    u16 ve_volume=0,ve_delta=0;std::array<u16,18> mix{};std::array<s16,9> dpop{};
    bool operator==(const DspVoiceMixState&)const=default;
};
struct DspVoiceSubframePcm {
    std::array<s16,32> input{},envelope{};std::array<std::array<s16,32>,9> lanes{};
    bool processed=false,ended_during_subframe=false;
};
class DspVoiceSubframe {
public:
    bool load(ByteView header,ByteView payload,u32 ratio,std::string* error){
        stream_={};linear_={};mix_={};running_=false;subframes_=zero_requests_=0;
        if(ratio>0x40000u)return fail(error,"type1 ratio outside qualified range");
        if(!stream_.load(header,payload,DspStreamMode::SourceAddresses,error))return false;
        linear_.ratio=ratio;if(error)error->clear();return true;
    }
    bool set_parameters(u16 ve,u16 delta,const std::array<u16,18>& mix,u32 ratio,std::string* error){
        if(!ready()||ratio>0x40000u)return fail(error,"loaded source and bounded ratio required");
        mix_.ve_volume=ve;mix_.ve_delta=delta;mix_.mix=mix;linear_.ratio=ratio;if(error)error->clear();return true;
    }
    bool start(std::string* error){if(!ready()||stream_.state().ended)return fail(error,"fresh or explicitly paused source required, not ended-source restart");running_=true;if(error)error->clear();return true;}
    bool stop(std::string* error){if(!ready())return fail(error,"loaded source required");running_=false;if(error)error->clear();return true;}
    bool render(DspVoiceSubframePcm* out,std::string* error){
        if(out)*out={};
        if(!out||!ready())return fail(error,"loaded source/output required");
        if(!running_){if(error)error->clear();return true;}
        if(subframes_==std::numeric_limits<u64>::max())return fail(error,"native subframe counter budget");
        auto cursor=stream_;const u32 needed=dsp_linear_detail::source_count(linear_,32);std::vector<s16> input;u32 produced=0;
        if(needed&&!cursor.read(needed,&input,&produced,error))return false;
        const u32 zeros=needed-produced;if(zero_requests_>std::numeric_limits<u64>::max()-zeros)return fail(error,"zero-request counter budget");
        input.resize(needed,0);DspLinearState next;std::vector<s16> resampled;
        if(!dsp_linear_detail::render(linear_,input,32,&next,&resampled))return fail(error,"bounded shared SRC kernel");
        DspVoiceSubframePcm result;result.processed=true;result.ended_during_subframe=cursor.state().ended;auto parameters=mix_;
        std::array<bool,9> enabled{};for(unsigned lane=0;lane<9;++lane)enabled[lane]=parameters.mix[lane*2]||parameters.mix[lane*2+1];
        for(unsigned i=0;i<32;++i){result.input[i]=resampled[i];const s32 volume=std::bit_cast<s16>(parameters.ve_volume);const s16 sample=scaled(resampled[i],volume);result.envelope[i]=sample;parameters.ve_volume=u16(parameters.ve_volume+parameters.ve_delta);
            for(unsigned lane=0;lane<9;++lane)if(enabled[lane]){const s16 value=scaled(sample,parameters.mix[lane*2]);result.lanes[lane][i]=value;parameters.dpop[lane]=value;parameters.mix[lane*2]=u16(parameters.mix[lane*2]+parameters.mix[lane*2+1]);}
        }
        stream_=std::move(cursor);linear_=next;mix_=parameters;running_=!result.ended_during_subframe;++subframes_;zero_requests_+=zeros;*out=result;if(error)error->clear();return true;
    }
    bool ready()const{return stream_.ready();}bool running()const{return ready()&&running_;}
    const DspLinearState& linear_state()const{return linear_;}const DspStreamState& source_state()const{return stream_.state();}
    const DspVoiceMixState& mix_state()const{return mix_;}u64 processed_subframes()const{return subframes_;}u64 zero_input_requests()const{return zero_requests_;}
private:
    static s16 scaled(s16 sample,s32 gain){return pcm_scale_q15_clipped(sample,gain);}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("DSP voice subframe: ")+why;return false;}
    DspStream stream_;DspLinearState linear_;DspVoiceMixState mix_;bool running_=false;u64 subframes_=0,zero_requests_=0;
};
} // namespace gf
