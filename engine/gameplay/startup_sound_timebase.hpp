#pragma once
// Native virtual GameCube timebase for source OSGetTick consumers. GC bus162MHz,
// TB=bus/4; low32 tick phase is native host-clock policy, NOT original hardware
// phase, simulation FPS or an audio-callback-derived clock. No sleep/device I/O.
#include <limits>
#include "engine/core/backends.hpp"
namespace gf {
struct StartupSoundTickSample {u32 tick=0,bus_clock=0;u64 host_ns=0;bool host_time_known=false;};
class StartupSoundTickSource {
public:
    virtual ~StartupSoundTickSource()=default;
    virtual bool sample(StartupSoundTickSample*,std::string*)=0;
};
enum class StartupSoundTimebaseMode {NativeGC162MHz};
inline constexpr u32 startup_sound_gc_tick_from_ns(u64 ns){
    // floor(ns*40500000/1e9) mod2^32, without overflowing ns*frequency.
    // Both products fit u64 for every input; division phase is never truncated
    // to milliseconds/frames. 40500000/1000000000 reduces exactly to81/2000.
    return u32((ns/2000u)*81u+((ns%2000u)*81u)/2000u);
}
class StartupSoundTimebase final:public StartupSoundTickSource {
public:
    explicit StartupSoundTimebase(IClock& clock):clock_(clock){}
    StartupSoundTimebase(const StartupSoundTimebase&)=delete;
    StartupSoundTimebase& operator=(const StartupSoundTimebase&)=delete;
    bool initialize(StartupSoundTimebaseMode mode,std::string* error){
        if(ready_||failed_||mode!=StartupSoundTimebaseMode::NativeGC162MHz)return fail(error,"fresh supported native timebase required");
        ready_=true;if(error)error->clear();return true;
    }
    bool sample(StartupSoundTickSample* out,std::string* error)override{
        if(out)*out={};
        if(!out||!ready_||failed_||samples_==std::numeric_limits<u64>::max())return fail(error,"initialized source clock/output/sample budget required");
        const u64 now=clock_.monotonic_ns();
        if(have_time_&&now<last_ns_){ready_=false;failed_=true;return fail(error,"native monotonic clock moved backwards");}
        last_ns_=now;have_time_=true;++samples_;*out={startup_sound_gc_tick_from_ns(now),162000000u,now,true};if(error)error->clear();return true;
    }
    bool ready()const{return ready_&&!failed_;}bool failed()const{return failed_;}u64 samples()const{return samples_;}
    void reset(){ready_=failed_=have_time_=false;last_ns_=samples_=0;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup sound native timebase: ")+why;return false;}
    IClock& clock_;bool ready_=false,failed_=false,have_time_=false;u64 last_ns_=0,samples_=0;
};
} // namespace gf
