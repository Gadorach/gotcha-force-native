#pragma once
// Native device-rate policy for a streaming wide stereo bus, not AX voice SRC.
// Rational32k->sink linear conversion, explicit two-sample lookahead, no clip.
#include <array>
#include <limits>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct PcmWideRateState {
    u32 target=0,phase=0;std::array<s32,2> first{},second{};
    u64 inputs=0,outputs=0;bool primed=false;
    bool operator==(const PcmWideRateState&)const=default;
};
class PcmWideRate {
public:
    bool load(u32 target){state_={};if(target<8000||target>192000)return false;state_.target=target;return true;}
    const PcmWideRateState& state()const{return state_;}
    template<class Pull> bool render(unsigned frames,Pull pull,std::vector<s64>* out,std::string* error){
        return render_positioned(frames,[&](std::array<s32,2>* value,unsigned,std::string* why){return pull(value,why);},out,error);
    }
    // The pull offset is a native production cut, not speaker latency: priming
    // uses0; lookahead reads after output i use i+1, possibly frames at block end.
    template<class Pull> bool render_positioned(unsigned frames,Pull pull,std::vector<s64>* out,std::string* error){
        if(out)out->clear();
        auto fail=[&](const char* why){if(error)*error=std::string("wide device rate: ")+why;return false;};
        if(!out||!state_.target||!frames||frames>4096||state_.outputs>std::numeric_limits<u64>::max()-frames)return fail("loaded rate/output/frame budget");
        unsigned producing_offset=0;
        auto next=state_;auto read=[&](std::array<s32,2>* value){if(next.inputs==std::numeric_limits<u64>::max())return false;if(!pull(value,producing_offset,error))return false;++next.inputs;return true;};
        if(!next.primed){if(!read(&next.first)||!read(&next.second))return false;next.primed=true;}
        std::vector<s64> result;result.reserve(frames*2);
        for(unsigned i=0;i<frames;++i){for(unsigned channel=0;channel<2;++channel){const s64 numerator=s64(next.first[channel])*(next.target-next.phase)+s64(next.second[channel])*next.phase;
                // Avoid overflow at full signed32 bus range and192k sink rate.
                const s64 value=(numerator/s64(next.target))*65536+(numerator%s64(next.target))*65536/s64(next.target);result.push_back(value);}
            const u32 phase=next.phase+32000;const unsigned steps=phase/next.target;next.phase=phase%next.target;
            producing_offset=i+1;
            for(unsigned j=0;j<steps;++j){next.first=next.second;if(!read(&next.second))return false;}
        }
        next.outputs+=frames;state_=next;*out=std::move(result);if(error)error->clear();return true;
    }
private:
    PcmWideRateState state_;
};
} // namespace gf
