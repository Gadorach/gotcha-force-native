#include <cstdio>
#include <cstdlib>
#include "engine/audio/dsp_voice_subframe.hpp"
#include "engine/audio/dsp_linear_window.hpp"
#include "tests/unit/dsp_stream_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"DSP voice subframe: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto h=dsp_stream_fixture::header(5),data=dsp_stream_fixture::payload(5);store_be16(h.data()+62,12);data[0]=12;DspVoiceSubframe voice;DspVoiceSubframePcm out;std::array<u16,18> mix{};mix[0]=32768;mix[2]=16384;mix[4]=65535;
    check(voice.load({h.data(),h.size()},{data.data(),data.size()},65536,&error),"owned five-sample source");check(voice.render(&out,&error)&&!out.processed&&voice.source_state().rendered==0,"loaded is not running; no fake start/advance");check(voice.set_parameters(32767,0,mix,65536,&error)&&voice.start(&error),"explicit VE/mix/start");
    check(voice.render(&out,&error)&&out.processed&&out.ended_during_subframe&&!voice.running(),"nonloop end latched during final32-sample subframe");
    check(out.input[0]==0&&out.input[1]==0&&out.input[2]==0&&out.input[3]==4096&&out.input[7]==4096&&out.input[8]==0&&out.input[31]==0,"four-history linear tail consumes latched zeros, not repeated compressed bytes");
    check(out.envelope[3]==4095&&out.envelope[4]==8191&&out.lanes[0][3]==4095&&out.lanes[1][3]==2047&&out.lanes[2][3]==8189,"signed VE then unsigned mix gain with floor at both cuts");
    check(voice.source_state().rendered==5&&voice.zero_input_requests()==27&&voice.linear_state().output_samples==32,"zero requests do not advance compressed histories/address");
    const auto linear=voice.linear_state();const auto source=voice.source_state();const auto parameters=voice.mix_state();check(voice.render(&out,&error)&&!out.processed&&voice.linear_state()==linear&&voice.source_state()==source&&voice.mix_state()==parameters,"next stopped subframe returns zero without advancing SRC/VE/mix");check(!voice.start(&error),"ended source cannot be restarted without explicit reload");
    DspLinearWindow strict;std::vector<s16> pcm;check(strict.load({h.data(),h.size()},{data.data(),data.size()},65536,&error)&&!strict.read_window(32,&pcm,&error)&&strict.source_state().rendered==0&&pcm.empty(),"old strict window retains whole-overread refusal");
    for(unsigned length=1;length<=96;++length)for(u32 ratio:{0x8000u,0x10000u,0x40000u}){auto header=dsp_stream_fixture::header(length),bytes=dsp_stream_fixture::payload(length);check(voice.load({header.data(),header.size()},{bytes.data(),bytes.size()},ratio,&error)&&voice.start(&error),"owned nonloop endpoint matrix");
        const u64 end_output=(u64(length)*65536+ratio-1)/ratio;const u64 expected_frames=(end_output+31)/32;unsigned frames=0;while(voice.running()){check(voice.render(&out,&error)&&out.processed,"active endpoint subframe");++frames;check(frames<=expected_frames,"backend stops at the first completed source-end subframe");}
        check(frames==expected_frames&&voice.source_state().rendered==length&&voice.linear_state().output_samples==expected_frames*32,"exact end phase/frame/source-read count");
    }
    // Nonzero decoder predictor histories survive source EOF; the SRC receives
    // zeros without decoding fabricated padding or resetting ADPCM state.
    h=dsp_stream_fixture::header(1);data=dsp_stream_fixture::payload(1);store_be16(h.data()+28,2048);store_be16(h.data()+64,10);check(voice.load({h.data(),h.size()},{data.data(),data.size()},65536,&error)&&voice.start(&error)&&voice.render(&out,&error)&&voice.source_state().history1==11&&voice.source_state().history2==10,"read-stop zero input does not run ADPCM predictor after endpoint");
    h=dsp_stream_fixture::header(28);data=dsp_stream_fixture::payload(28);store_be16(h.data()+12,1);store_be16(h.data()+62,12);store_be16(h.data()+68,12);for(unsigned i=0;i<data.size();i+=8)data[i]=12;
    check(voice.load({h.data(),h.size()},{data.data(),data.size()},65536,&error)&&voice.set_parameters(65535,0,mix,65536,&error)&&voice.start(&error)&&voice.render(&out,&error)&&voice.running()&&voice.zero_input_requests()==0,"loop resets compressed context and never trips nonloop stop");check(out.envelope[3]==-1&&out.lanes[0][3]==-1,"GC VEffff is signed-1, not positive65535");
    mix.fill(0);mix[0]=65535;mix[1]=1;check(voice.set_parameters(65535,1,mix,65536,&error)&&voice.render(&out,&error)&&voice.mix_state().ve_volume==31&&voice.mix_state().mix[0]==31,"VE and enabled mix deltas wrap16 per sample");
    mix.fill(0);mix[0]=32768;check(voice.set_parameters(32767,0,mix,65536,&error)&&voice.render(&out,&error),"nonzero dpop lane input");const auto dpop=voice.mix_state().dpop[0];mix.fill(0);check(voice.set_parameters(32767,0,mix,65536,&error)&&voice.render(&out,&error)&&voice.mix_state().dpop[0]==dpop,"disabled lane retains source dpop rather than inventing clear");
    auto state=voice.linear_state();check(voice.stop(&error)&&voice.render(&out,&error)&&!out.processed&&voice.linear_state()==state&&voice.start(&error),"explicit stop/resume retains source cursor, no implicit rewind");
    check(voice.set_parameters(32767,0,mix,0,&error)&&voice.render(&out,&error)&&voice.running(),"ratio0 source hold does not fake completion");state=voice.linear_state();check(!voice.set_parameters(1,1,mix,0x40001,&error)&&voice.linear_state()==state,"invalid parameter ratio refuses without partial state update");
    DspVoiceSubframe moved=std::move(voice);check(!voice.ready()&&!voice.render(&out,&error)&&!out.processed&&moved.ready(),"moved-from voice cannot replay frames");check(!moved.load({h.data(),h.size()},{data.data(),data.size()},0x40001,&error)&&!moved.ready(),"failed load invalidates old voice/source binding");
    std::printf("{\"dsp_voice_subframe_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
