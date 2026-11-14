#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/source_setup_input.hpp"
#include "engine/gameplay/static_record_scene.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"Story shared setup reuse: %s\n",why);std::abort();}}}
int main(){using namespace gf;constexpr s8 mapping[32]={0,1,2,3,4,5,6,-1,8,9,10,11,7,-1,-1,-1,0,1,2,3,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1};
    for(u32 i=0;i<65536;++i){const u32 word=i|(i*0x9e370001u);u16 expected=0;for(unsigned bit=0;bit<32;++bit)if((word&(1u<<bit))&&mapping[bit]>=0)expected|=u16(1u<<unsigned(mapping[bit]));check(source_setup_fold_buttons(word)==expected,"source bit aliases/Start fold, no MAIN repeat substitution");}
    for(unsigned bit=0;bit<16;++bit){SourceSetupButtons buttons;const u32 word=1u<<(bit==7?12:bit<12?bit:bit+8);
        for(unsigned tick=1;tick<=80;++tick){source_setup_input_step({word,true},&buttons);const u16 mask=u16(1u<<bit);const bool repeat=tick==1||(tick>=32&&(tick-32)%5==0);check(buttons.held==mask&&buttons.pressed==(tick==1?mask:0)&&buttons.repeated==(repeat?mask:0),"carried history and32/27 repeat");}
        source_setup_input_step({word,false},&buttons);check(!buttons.held&&!buttons.pressed&&!buttons.repeated&&buttons.counters==std::array<u8,16>{},"disconnect resets held/counters without fake edge");
        source_setup_input_step({word,true},&buttons);check(buttons.pressed==u16(1u<<bit),"reconnect new edge");
    }
    GamePadState pad;pad.start=true;pad.x=true;const auto digital=source_setup_digital_pad(pad,true);check(digital.source_buttons==0x1400&&digital.connected&&source_setup_fold_buttons(digital.source_buttons)==0x480,"native digital adapter only");
    const auto bytes=hsd_record_animation_fixture::Fixture{}.archive();HsdRecordAnimationBank bank;std::string error;check(bank.load({bytes.data(),bytes.size()},&error),"owned source bank");RecordingRenderBackend backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"shared static icons"},nullptr,&error),"headless render");
    Camera camera;camera.near_plane=.1f;camera.far_plane=100;camera.fov_y_degrees=60;const f32 position[3]={0,0,-10};const auto world=mat4_translation(position);std::map<u16,AnimatedModelScene> cache;
    auto frame=[&](f32 alpha,std::array<u8,3> rgb,bool always){render.begin_frame({0,0,0,0});render.reset_arena();render.state()={};const StaticRecordAppearance appearance{alpha,rgb};check(draw_static_record(render,bank,cache,0,world,camera,50000,&error,always,true,&appearance),error.c_str());check(render.end_frame(),"actual submitted static records");render.present();check(backend.draws()>0&&backend.last_frame_digest()!=0,"nonempty recorded draws published after present");return backend.last_frame_digest();};
    (void)frame(.5f,{255,255,255},false);const auto stable=frame(.5f,{255,255,255},false);check(frame(.5f,{255,255,255},false)==stable&&cache.size()==1,"retained per-bank cache, unchanged repeat draw");
    check(frame(1,{255,255,255},false)!=stable,"explicit static alpha uses old shared material path");check(frame(.5f,{128,255,255},false)!=stable,"source icon multiplication uses old shared path");check(frame(.5f,{255,255,255},true)!=stable,"explicit compare override");check(frame(.5f,{255,255,255},false)==stable,"compare override restored");
    const StaticRecordAppearance invalid{std::numeric_limits<f32>::quiet_NaN(),{255,255,255}};check(!draw_static_record(render,bank,cache,0,world,camera,50000,&error,false,true,&invalid),"invalid alpha refuses");cache.clear(); //caller contract: retire/rebind cache after failure or bank/camera/namespace replacement
    (void)frame(.5f,{255,255,255},false);render.shutdown();
    std::printf("Story collected setup kernels: %u owned checks PASS; Challenge source reuse, no selector policy/GPU/device ACK\n",checks);
}
