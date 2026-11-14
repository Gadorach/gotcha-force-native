#pragma once
// Bounded source-type1 linear SRC window on the shared compressed DSP cursor.
// Q16 ratio is input samples per 32kHz output sample. Four source history values
// persist across chunks/rate changes, including initial zero-history latency.
// Nonloop backend silence-address/final-DSP-block behavior is NOT synthesized.
#include "engine/audio/dsp_stream.hpp"
#include "engine/audio/dsp_linear_kernel.hpp"
namespace gf {
class DspLinearWindow {
public:
    bool load(ByteView header,ByteView payload,u32 ratio,std::string* error){
        stream_={};state_={};
        if(ratio>0x40000u)return fail(error,"type1 Q16 ratio outside qualified backend range");
        if(!stream_.load(header,payload,DspStreamMode::SourceAddresses,error))return false;
        state_.ratio=ratio;if(error)error->clear();return true;
    }
    bool set_ratio(u32 ratio,std::string* error){
        if(!ready()||ratio>0x40000u)return fail(error,"loaded cursor and bounded type1 ratio required");
        state_.ratio=ratio;if(error)error->clear();return true;
    }
    bool restart(){if(!stream_.restart())return false;const u32 ratio=state_.ratio;state_={};state_.ratio=ratio;return true;}
    bool read_window(unsigned requested,std::vector<s16>* out,std::string* error){
        if(out)out->clear();
        if(!out||!ready()||!requested||requested>4096||state_.output_samples>std::numeric_limits<u64>::max()-requested)return fail(error,"loaded output/window/counter budget");
        const u32 needed=dsp_linear_detail::source_count(state_,requested);
        auto cursor=stream_;std::vector<s16> input;input.reserve(needed);
        while(input.size()<needed){const unsigned count=std::min(4096u,needed-u32(input.size()));std::vector<s16> block;u32 produced=0;
            if(!cursor.read(count,&block,&produced,error))return false;
            if(produced!=count)return fail(error,"nonloop source window exhausted; backend silence/tail consumer required");
            input.insert(input.end(),block.begin(),block.end());
        }
        DspLinearState next;std::vector<s16> result;if(!dsp_linear_detail::render(state_,input,requested,&next,&result))return fail(error,"derived source-window count/state");
        stream_=std::move(cursor);state_=next;*out=std::move(result);if(error)error->clear();return true;
    }
    bool ready()const{return stream_.ready();}
    const DspLinearState& state()const{return state_;}
    const DspStreamState& source_state()const{return stream_.state();}
    u32 source_end_sample()const{return stream_.end_sample();}
    const DspAdpcmInfo* info()const{return stream_.info();}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("DSP linear window: ")+why;return false;}
    DspStream stream_;DspLinearState state_;
};
} // namespace gf
