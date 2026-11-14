#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_popup_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/hsd_shape_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"popup scene: %s\n",why);std::abort();}}
struct Backend final:gf::IRenderBackend {
    std::vector<gf::Vertex> vertices;std::vector<float> depths;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex* p,std::size_t n)override{vertices.assign(p,p+n);return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{depths.clear();}
    void submit(const gf::DrawCall& call)override{depths.push_back(vertices.at(call.vertex_first).z);}
    void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;auto fixture=hsd_shape_fixture::fixture();fixture.word(0x244,0x60002001);const auto bytes=fixture.archive();HsdRecordAnimationBank bank;std::string error;
    check(bank.load({bytes.data(),bytes.size()},&error),"owned translucent shape bank");ForcePopupArt art;art.bank_hash=bank.hash();art.open_limit=25;art.visible_after=8;art.menu_z=-1;art.confirm_z=0;art.angle_unit=.001f;
    for(auto& m:art.menus){m.labels={0};m.widgets[0].record=0;}art.confirmation[0].record=0;
    Camera camera;camera.eye[2]=5;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;camera.viewport_width=camera.viewport_height=128;
    Backend backend;headless::Window window;RenderEngine render(backend);check(render.initialize({128,128,false,true,"owned popup"},&window,&error),"recording renderer");ForcePopupScene scene;check(scene.load(bank,art,camera),"popup binding");
    ForcePopupPresentation near,far;near.kind=ForcePopupKind::Erase;near.selected=1;near.frame=2;far.frame=1;
    const std::array<ForcePopupPresentation,2> reverse={near,far},forward={far,near};
    render.begin_frame({});render.reset_arena();check(scene.draw_many(render,reverse,&error)&&render.end_frame(),"overlap submits through shared passes");
    check(backend.depths.size()==2&&backend.depths[0]<backend.depths[1],"translucent retiring/opening panels globally sort far to near");const auto expected=backend.depths;
    render.begin_frame({});render.reset_arena();check(scene.draw_many(render,forward,&error)&&render.end_frame()&&backend.depths==expected,"input panel order does not override geometric depth");
    auto invalid=reverse;invalid[1].selected=8;render.begin_frame({});render.reset_arena();check(!scene.draw_many(render,invalid,&error)&&render.pending_draws()==0&&render.end_frame()&&backend.depths.empty(),"late invalid presentation rejected before any draw");
    render.begin_frame({});render.reset_arena();check(scene.draw_many(render,{},&error)&&render.end_frame()&&backend.depths.empty(),"no modal emits no geometry");
    art.bank_hash^=1;check(!scene.load(bank,art,camera),"foreign profile clears owner");render.begin_frame({});render.reset_arena();check(!scene.draw(render,far,&error)&&render.end_frame()&&backend.depths.empty(),"failed load cannot replay cached panels");
    std::printf("popup scene: %u owned checks PASS\n",checks);
}
