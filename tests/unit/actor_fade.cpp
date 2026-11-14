#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/actor_fade_fixture.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "engine/graphics/render_engine.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"actor fade: %s\n",why);std::abort();}}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;using namespace actor_fade_fixture;
    auto bytes=profile();ActorFadeProfile p;std::string error;check(actor_fade_load({bytes.data(),bytes.size()},&p,&error),"owned two-curve profile");
    ActorFadeState s,next;check(actor_fade_initialize(p,&s)&&s.material_rgba==255&&!s.channels[0].active&&!s.channels[1].active,"fresh wrapper black/opaque, not transparent");
    check(actor_fade_request(p,s,14,1,&s)&&s.material_rgba==255&&s.channels[1].rgba==255,"request stores black and time0 without sampling or changing wrapper");
    const std::array<u32,5> expected={0xff8040ffu,0xbf6030ffu,0x7f4020ffu,0x3f2010ffu,0x000000ffu};
    for(unsigned i=0;i<5;++i){check(actor_fade_update(p,s,1,&s)&&s.material_rgba==expected[i]&&s.channels[1].time==f32(i+1)&&s.channels[1].active==(i<4),"sample before advancing clock, truncate bytes, terminal black theninactive");}
    const auto stopped=fields(s);check(actor_fade_update(p,s,1,&s)&&fields(s)==stopped,"inactive channels retain output/time, not replay or invented clock advance");
    check(actor_fade_initialize(p,&s)&&actor_fade_request(p,s,12,0,&s),"owned colored loop request");
    for(unsigned i=0;i<1025;++i){check(actor_fade_update(p,s,1,&s)&&s.channels[0].time==f32(i%4+1)&&s.channels[0].loops==u8(i/4),"loop subtraction and modulo256 loop counter, not stored cursor drift");}
    check(s.material_rgba==0x0a50c8ffu&&s.channels[0].loops==0,"loop count wraps after256cycles");
    check(actor_fade_request(p,s,14,1,&s),"higher-priority channel request");
    for(unsigned i=0;i<5;++i)check(actor_fade_update(p,s,1,&s)&&s.material_rgba==expected[i],"channel1 processed afterchannel0, including its terminal update");
    check(actor_fade_update(p,s,1,&s)&&s.material_rgba==0x3c3c96ffu,"remaining channel0 resumes material output afterchannel1stops");
    const auto old_time=s.channels[0].time;const auto old_colour=s.channels[0].rgba;check(actor_fade_clear(p,s,&s)&&s.material_rgba==255&&!s.channels[0].active&&s.channels[0].time==old_time&&s.channels[0].rgba==old_colour,"clear disables both pointers/resets wrapper only; channel clocks/colors retained");
    check(actor_fade_request(p,s,12,0,&s)&&actor_fade_update(p,s,16,&s)&&actor_fade_update(p,s,0,&s)&&s.channels[0].loops==4&&s.channels[0].time==0,"large bounded rate can wrap multiple periods, zero rate samples without advancing");
    check(actor_fade_request(p,s,14,1,&s),"reset finite curve forzero delta");for(unsigned i=0;i<8;++i)check(actor_fade_update(p,s,0,&s)&&s.channels[1].time==0&&s.material_rgba==0xff8040ffu,"zero actor delta freezes time but still samples");
    auto special=bytes;store_be32(special.data()+72,0x112233ffu);ActorFadeProfile terminal;check(actor_fade_load({special.data(),special.size()},&terminal,&error)&&actor_fade_initialize(terminal,&next)&&actor_fade_request(terminal,next,14,1,&next),"owned nonblack terminal control");
    for(unsigned i=0;i<5;++i)check(actor_fade_update(terminal,next,1,&next),"nontrapping terminal conversion modeled without C++NaNcast");
    check(next.material_rgba==255,"zero-span terminal yields invalid-conversion lowbytes0, not copied endpointRGB");
    ActorFadeMaterial material;check(actor_fade_material(p,s,&material)&&material.source_class&&material.rgb==std::array<u8,3>{255,128,64},"material consumes RGB only, base alpha untouched");
    Recorder backend;RenderEngine render(backend);std::array<Vertex,3> triangle{};render.upload_vertices(triangle.data(),triangle.size());render.begin_frame({});render.state().post_rgb_add=material.rgb;check(render.draw(Primitive::Triangles,0,3),"fade draw captured");render.state().post_rgb_add={};render.end_frame();check(backend.calls.size()==1&&backend.calls[0].post_rgb_add==material.rgb,"perdraw RGB snapshot survives later state changes");
    const auto unchanged=fields(s);check(!actor_fade_update(p,s,NAN,&next)&&!actor_fade_request(p,s,13,1,&next)&&!actor_fade_request(p,s,14,2,&next),"nonfinite delta/unknown curve/channel refuses");next=s;check(!actor_fade_update(p,s,17,&next)&&fields(next)==unchanged,"failure leaves previous destination unchanged, caller mustnot replay");
    auto bad=s;bad.profile_binding^=1;check(!actor_fade_material(p,bad,&material)&&!actor_fade_update(p,bad,1,&next),"cross-profile lifecycle refuses");bad=s;bad.channels[0].time=NAN;check(!actor_fade_update(p,bad,1,&next),"nonfinite retained clock refuses");
    for(unsigned at:{0u,4u,6u,8u,12u,16u,20u,24u,25u,28u,35u,36u,48u,52u,53u,56u,63u,64u,76u}){auto malformed=bytes;malformed[at]^=1;ActorFadeProfile rejected=p;check(!actor_fade_load({malformed.data(),malformed.size()},&rejected,&error)&&!rejected.loaded,"unreviewed header/curveidentity/start/control/alpha rejects atomically");}
    for(u32 end:{0u,0x7fc00000u,0xbf800000u}){auto malformed=bytes;store_be32(malformed.data()+40,end);check(!actor_fade_load({malformed.data(),malformed.size()},&p,&error),"invalid curve endpoint refuses");}
    check(actor_fade_load({bytes.data(),bytes.size()},&p,&error),"restore owned profile");bytes.assign(bytes.size(),0);check(actor_fade_initialize(p,&s)&&actor_fade_request(p,s,14,1,&s)&&actor_fade_update(p,s,1,&s),"curve owner survives caller buffer destruction");
    unsigned actual=0;std::ifstream file("local/content/actor-fade/startup.gfaf",std::ios::binary);std::vector<u8> raw{std::istreambuf_iterator<char>(file),{}};
    if(!raw.empty()){check(actor_fade_load({raw.data(),raw.size()},&p,&error)&&p.curves[0].end==99&&p.curves[0].loop&&p.curves[1].end==30&&!p.curves[1].loop,"actual source curve extents/control, not following neighboring table data");
        check(actor_fade_initialize(p,&s)&&actor_fade_request(p,s,14,1,&s),"source reset14request");for(unsigned tick=0;tick<=31;++tick)check(actor_fade_update(p,s,1,&s)&&((tick==0&&s.material_rgba==0xffffffffu)||(tick>0&&tick<30)||(tick>=30&&s.material_rgba==255&&!s.channels[1].active)),"actual finite reset curve reachesblack/stops");
        check(actor_fade_request(p,s,12,1,&s),"source exit12request");for(unsigned tick=0;tick<300;++tick)check(actor_fade_update(p,s,1,&s)&&s.material_rgba==0xffffffffu&&s.channels[1].active,"actual loop12white remainsactive, no forced fade-to-zero");check(s.channels[1].loops==3,"actual99period loops");actual=1;
    }
    std::printf("{\"actor_fade_checks\":%u,\"source_profiles\":%u,\"rgb_add_not_opacity\":true,\"actor_timing_or_visible_startup_admitted\":false,\"all_passed\":true}\n",checks,actual);
}
