#pragma once
#include <cstdlib>
#include <deque>
#include "engine/audio/pcm_queue.hpp"
namespace pcm_voice_slots_fixture {
using namespace gf;
struct Sink final:IAudioBackend {
    bool present=true,exact=true,reject_clear=false,accept_but_fail=false;unsigned submits=0,clears=0,fail_at=0,race=0;u64 consumed=0,discarded=0;
    std::deque<std::array<s16,2>> queued;std::vector<std::array<s16,2>> heard;
    bool open(unsigned,unsigned)override{return true;}void close()override{present=false;}bool available()const override{return present;}
    void submit(const s16*,std::size_t)override{std::abort();}u64 frames_consumed()const override{return consumed;}u64 frames_available()const override{return queued.size();}u64 frames_discarded()const override{return discarded;}
    void consume(unsigned n){while(n--&&!queued.empty()){heard.push_back(queued.front());queued.pop_front();++consumed;}}
    bool try_submit(const s16* p,std::size_t n)override{++submits;const bool fail=fail_at&&submits==fail_at;if(!fail||accept_but_fail)for(unsigned i=0;i<n;++i)queued.push_back({p[i*2],p[i*2+1]});return !fail;}
    bool discard_queued()override{std::abort();}bool discard_queued_exact(u64* out)override{++clears;if(!out||!exact||reject_clear)return false;consume(race);race=0;*out=queued.size();discarded+=*out;queued.clear();return true;}
};
inline std::vector<s64> weighted(unsigned frames,s64 left,s64 right){std::vector<s64> v(std::size_t(frames)*2);for(unsigned i=0;i<frames;++i){v[i*2]=left;v[i*2+1]=right;}return v;}
inline s16 encoded(s64 wide){return s16(std::clamp<s64>(wide/65536,-32768,32767));}
}
