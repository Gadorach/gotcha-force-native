#pragma once
// Incremental Nintendo DSP compressed sample cursor. Source EDE6C passes the
// header loop predictor/histories and nibble-address bounds to the voice backend.
// This is a codec primitive, NOT voice allocation, pitch, envelope or audibility.
#include <limits>
#include <memory>
#include "engine/assets/dsp_adpcm.hpp"
namespace gf {
enum class DspStreamMode {DeclaredOnce,HeaderPlayback,SourceAddresses};
struct DspStreamState {
    u32 position=0;s32 history1=0,history2=0;u64 rendered=0,loops=0;bool ended=false;
    bool operator==(const DspStreamState&)const=default;
};
class DspStream {
public:
    bool load(ByteView header,ByteView payload,DspStreamMode mode,std::string* error){
        data_.reset();state_={};
        if(mode!=DspStreamMode::DeclaredOnce&&mode!=DspStreamMode::HeaderPlayback&&mode!=DspStreamMode::SourceAddresses)return fail(error,"explicit playback mode required");
        auto data=std::make_shared<Data>();if(!(mode==DspStreamMode::SourceAddresses?dsp_voice_header(header,&data->info,error):dsp_adpcm_header(header,&data->info,error)))return false;const auto& info=data->info;
        if(!payload.data||payload.size!=dsp_detail::payload_bytes(info.nibbles)||payload.data[0]!=info.initial_ps)return fail(error,"payload/initial predictor binding");
        if(info.loop&&payload.data[(header.be32(16)/16)*8]!=info.loop_ps)return fail(error,"loop predictor binding");
        data->end=mode==DspStreamMode::DeclaredOnce?info.frames:dsp_detail::sample_of(header.be32(20))+1;
        const u64 limit=mode==DspStreamMode::SourceAddresses?(payload.size/8)*14:u64(info.frames);
        if(!data->end||data->end>limit)return fail(error,"source address end domain");
        // Old modes retain whole-declared-frame validation. Explicit source
        // addresses validate every accessed stored frame, not unvisited padding.
        const u32 validated=mode==DspStreamMode::SourceAddresses?data->end:info.frames;
        for(u32 frame=0;frame<validated;frame+=14)if(payload.data[(frame/14)*8]>>4>=8)return fail(error,"invalid frame predictor");
        data->payload.assign(payload.data,payload.data+payload.size);data->repeat=mode!=DspStreamMode::DeclaredOnce&&info.loop;
        data_=std::move(data);restart();if(error)error->clear();return true;
    }
    bool restart(){if(!data_)return false;state_={};state_.history1=data_->info.initial_h1;state_.history2=data_->info.initial_h2;return true;}
    bool read(unsigned requested,std::vector<s16>* out,u32* produced,std::string* error){
        if(out)out->clear();
        if(produced)*produced=0;
        if(!data_||!out||!requested||requested>4096||state_.rendered>std::numeric_limits<u64>::max()-requested)return fail(error,"loaded cursor/output/block budget required");
        auto next=state_;std::vector<s16> samples;samples.reserve(requested);const auto& info=data_->info;
        for(unsigned i=0;i<requested&&!next.ended;++i){const u32 frame=next.position/14,index=next.position%14;const u8 ps=data_->payload[frame*8],byte=data_->payload[frame*8+1+index/2];const unsigned bits=index&1?byte&15:byte>>4,predictor=ps>>4;
            samples.push_back(dsp_detail::sample(bits,s64(1)<<(ps&15),info.coefficients[predictor*2],info.coefficients[predictor*2+1],next.history1,next.history2));++next.position;++next.rendered;
            if(next.position==data_->end){if(data_->repeat){if(next.loops==std::numeric_limits<u64>::max())return fail(error,"loop counter budget");++next.loops;next.position=info.loop_start;next.history1=info.loop_h1;next.history2=info.loop_h2;}else next.ended=true;}
        }
        state_=next;if(produced)*produced=u32(samples.size());*out=std::move(samples);if(error)error->clear();return true;
    }
    bool ready()const{return bool(data_);}const DspAdpcmInfo* info()const{return data_?&data_->info:nullptr;}
    const DspStreamState& state()const{return state_;}u32 end_sample()const{return data_?data_->end:0;}
private:
    struct Data {DspAdpcmInfo info;std::vector<u8> payload;u32 end=0;bool repeat=false;};
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("DSP stream: ")+why;return false;}
    std::shared_ptr<const Data> data_;DspStreamState state_;
};
} // namespace gf
