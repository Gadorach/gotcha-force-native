#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_name_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/force_name_fixture.hpp"
#include "tests/unit/hsd_record_texture_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"name render: %s\n",why);std::abort();}}
struct Backend final:IRenderBackend {
    std::vector<Vertex> vertices;std::vector<DrawCall> draws;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}void shutdown()override{}RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(u32,const TextureDesc&)override{return true;}bool upload_matrix(u32,const f32*)override{return true;}
    void begin_frame(const FrameDesc&)override{draws.clear();}void submit(const DrawCall& d)override{draws.push_back(d);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;std::string error;auto font=force_name_fixture::font();Backend backend;RenderEngine render(backend);headless::Window window;check(render.initialize({640,480,false,true,"owned"},&window,&error),"renderer");SourceTextRenderer text;check(text.load(font,render,1),"font");
    render.begin_frame({});render.reset_arena();render.state().texture=99;render.state().depth_write=true;
    check(text.draw_screen(render,std::string("\x82\x60\x81\x40\x82\x61",6),133,90,30,19,1,480.f/448,0,0xffffffff)&&render.state().texture==99&&render.state().depth_write,"screen text restores render state");check(render.end_frame()&&backend.vertices.size()==12&&backend.draws.size()==1&&!backend.draws[0].depth_test&&!backend.draws[0].depth_write,"only two visible glyphs, overlay depth policy");
    f32 minx=1e6f,maxx=-1e6f,miny=1e6f,maxy=-1e6f;for(const auto& v:backend.vertices){minx=std::min(minx,v.x);maxx=std::max(maxx,v.x);miny=std::min(miny,v.y);maxy=std::max(maxy,v.y);}
    check(minx==133&&maxx==201&&std::abs(miny-90*480.f/448)<.0001f&&std::abs(maxy-120*480.f/448)<.0001f,"source top-left/blank advance/640x448 scale");
    render.begin_frame({});render.reset_arena();check(!text.draw_screen(render,std::string("\x82\x60\x82\x62",4),0,0,30,19,1,1,0,0xffffffff)&&render.pending_draws()==0&&render.arena_size()==0,"late unknown glyph atomic refusal");check(!text.draw_screen(render,"",NAN,0,30,19,1,1,0,0xffffffff),"NaN placement refuses");check(render.begin_model_pass(2)&&!text.draw_screen(render,"",0,0,30,19,1,1,0,0xffffffff)&&render.end_model_pass()&&render.end_frame(),"screen glyphs require overlay boundary");
    AttachmentAlternateProfile style;style.words={0x20080000,0x50140000,4,10,0x60002001,0x400a,0x20,7};style.alpha=.4f;style.loaded=true;
    auto fixture=hsd_record_texture_fixture::fixture();
    // Remove material AObjs while retaining original UV AObj on mesh0.
    for(unsigned i=0;i<3;++i){const unsigned field=0x904+i*32;fixture.word(field,0);fixture.fields.erase(std::remove(fixture.fields.begin(),fixture.fields.end(),field),fixture.fields.end());}
    auto bytes=fixture.archive();HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error),"owned animated texture bank");HsdRecordAnimationBinding normal,alternate;check(bank.select_textures(0,2,0,0,&normal,&error)&&bank.select_alternate_textures(0,2,0,0,style,&alternate,&error),"opt-in alternate texture binding");
    check(normal.scene()->meshes[0].render_mode==0x201d&&normal.scene()->meshes[0].material_alpha==.75f&&alternate.scene()->meshes[0].render_mode==0x60002015&&alternate.scene()->meshes[0].material_alpha==.4f,"immutable original and separately styled copy");
    AnimatedModelScene model;check(model.load_alternate_textures(bank,0,2,0,0,style,&error)&&model.request(0,0,&error)&&model.replace_unanimated_alpha(1,&error),"separate alpha with UV animation");check(model.request_again(2,0,&error)&&model.pose()->materials[0].uv_offset[0]==.375f&&model.pose()->materials[0].alpha==1,"UV callback keeps separately owned alpha");
    check(!model.replace_static_alpha(.5f,&error)&&!model.ready(),"old static-alpha refusal retained");
    auto active=hsd_record_texture_fixture::fixture();bytes=active.archive();HsdRecordAnimationBank live;check(live.load({bytes.data(),bytes.size()},&error)&&model.load_alternate_textures(live,0,2,0,0,style,&error)&&model.request(0,0,&error),"live alpha fixture");check(!model.replace_unanimated_alpha(.5f,&error)&&!model.ready(),"live alpha callback cannot be overridden");
    auto invalid=style;invalid.loaded=false;check(!bank.select_alternate_textures(0,2,0,0,invalid,&alternate,&error)&&!alternate.scene(),"invalid style clears binding");
    std::printf("name shared rendering: %u owned checks PASS\n",checks);
}
