#include <cstdio>
#include <cstdlib>
#include "engine/audio/dsp_linear_window.hpp"
#include "tests/unit/dsp_stream_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"DSP linear window: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto h=dsp_stream_fixture::header(28),payload=dsp_stream_fixture::payload(28);DspLinearWindow window;std::vector<s16> out;
    check(window.load({h.data(),h.size()},{payload.data(),payload.size()},65536,&error),"owned mono DSP + explicit type1 ratio");check(window.read_window(8,&out,&error)&&out==std::vector<s16>({0,0,0,1,2,1,2,1}),"source zero-history latency uses oldest pair, not immediate/newest interpolation");check(window.source_state().rendered==8&&window.state().fraction==0&&window.state().history==std::array<s16,4>{1,2,1,2},"four-source history and compressed cursor advance together");
    check(window.restart()&&window.state().ratio==65536&&window.read_window(1,&out,&error)&&out==std::vector<s16>{0},"explicit restart resets history/fraction but retains ratio");
    check(window.set_ratio(32768,&error)&&window.read_window(7,&out,&error)&&out==std::vector<s16>({0,0,0,0,0,1,1}),"half-rate fractional source history retained across rate change");
    auto before=window.state();auto source=window.source_state();check(!window.set_ratio(0x40001,&error)&&window.state()==before&&window.source_state()==source,"out-of-range ratio refuses without clearing valid cursor");
    check(!window.read_window(0,&out,&error)&&out.empty()&&window.state()==before,"zero window refuses without old-output replay");check(!window.read_window(4097,&out,&error)&&out.empty()&&window.state()==before,"bounded output size");
    check(!window.read_window(4096,&out,&error)&&out.empty()&&window.state()==before&&window.source_state()==source,"late nonloop source-window exhaustion rolls back all decoder/SRC state");
    check(window.set_ratio(0,&error)&&window.read_window(16,&out,&error)&&window.source_state()==source&&window.state().history==before.history,"ratio0 is a source hold, not invented nearest/no-SRC fallback");
    // A loop starts mid-frame and restores ADPCM header state, independently of
    // the retained SRC history. Both whole and fragmented windows must agree.
    store_be16(h.data()+12,1);store_be32(h.data()+16,9);store_be32(h.data()+20,24);
    for(u32 ratio:{0u,1u,0x4000u,0x8000u,0xffffu,0x10000u,0x18000u,0x3ffffu,0x40000u}){
        DspLinearWindow whole,parts;check(whole.load({h.data(),h.size()},{payload.data(),payload.size()},ratio,&error)&&parts.load({h.data(),h.size()},{payload.data(),payload.size()},ratio,&error),"owned loop ratio case");std::vector<s16> expected,actual;check(whole.read_window(257,&expected,&error),"whole SRC window");
        unsigned total=0;while(total<257){const unsigned n=std::min(257-total,(total%17)+1);check(parts.read_window(n,&out,&error),"fragmented SRC window");actual.insert(actual.end(),out.begin(),out.end());total+=n;}
        check(expected==actual&&whole.state()==parts.state()&&whole.source_state()==parts.source_state(),"partition-independent output/fraction/history/compressed-loop state");
    }
    DspLinearWindow varying;check(varying.load({h.data(),h.size()},{payload.data(),payload.size()},0,&error),"new varying-rate loop");
    for(unsigned i=0;i<128;++i){const u32 ratio=(i*31337u)%0x40001u;check(varying.set_ratio(ratio,&error)&&varying.read_window(31,&out,&error)&&out.size()==31&&varying.state().fraction<65536,"time-varying bounded Q16 rate keeps fractional phase");}
    // Negative interpolation must floor, not truncate toward zero. Owned
    // predictor0/shift0 nibbles F,1 represent -1,+1 alternating.
    auto negative=payload;for(unsigned i=0;i<negative.size();++i)negative[i]=i%8?0xf1:0;check(window.load({h.data(),h.size()},{negative.data(),negative.size()},32768,&error)&&window.read_window(12,&out,&error),"owned negative linear window");check(out==std::vector<s16>({0,0,0,0,0,0,-1,-1,0,1,0,-1}),"signed interpolation floors small negative fractional values");
    DspLinearWindow copy=window;check(copy.read_window(64,&out,&error),"independent cursor copy shares immutable compressed data");check(copy.state()!=window.state(),"copy does not advance original cursor");DspLinearWindow moved=std::move(copy);check(!copy.ready()&&!copy.read_window(1,&out,&error)&&out.empty()&&moved.ready(),"moved-from cursor cannot replay data");
    check(!window.load({h.data(),h.size()},{payload.data(),payload.size()},0x40001,&error)&&!window.ready()&&!window.read_window(1,&out,&error),"failed load invalidates old sample binding");
    auto nonloop=dsp_stream_fixture::header(5),small=dsp_stream_fixture::payload(5);check(window.load({nonloop.data(),nonloop.size()},{small.data(),small.size()},65536,&error)&&window.read_window(5,&out,&error)&&window.source_state().ended,"exact final bounded window records compressed-source exhaustion");before=window.state();source=window.source_state();check(!window.read_window(1,&out,&error)&&out.empty()&&window.state()==before&&window.source_state()==source,"nonloop silence-address/final-subframe tail is not fabricated");
    std::printf("{\"dsp_linear_window_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
