#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"model depth compare: %s\n",why);std::abort();}}
struct Backend final:gf::IRenderBackend {
    std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;hsd_record_animation_fixture::Fixture fixture;auto bytes=fixture.archive();HsdRecordAnimationBank bank;AnimatedModelScene scene;std::string error;check(bank.load({bytes.data(),bytes.size()},&error)&&scene.load(bank,0,0,0,0,&error)&&scene.request(0,0,&error),"owned model");scene.set_enabled(true);
    Backend backend;headless::Window window;RenderEngine render(backend);check(render.initialize({128,128,false,true,"depth"},&window,&error),"recording renderer");Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;camera.viewport_width=camera.viewport_height=128;
    auto draw=[&](){render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==3&&render.end_frame(),"three owned meshes");};
    draw();const auto baseline=backend.calls;check(baseline.size()==3&&baseline[0].depth_compare!=7&&baseline[0].depth_write,"owned baseline compares and writes");
    check(scene.set_depth_compare(7),"source ALWAYS compare");draw();for(unsigned i=0;i<3;++i){const auto& actual=backend.calls[i];const auto& old=baseline[i];check(actual.depth_compare==7&&actual.depth_write==old.depth_write&&actual.depth_test==old.depth_test&&actual.blend==old.blend&&actual.colour_write==old.colour_write&&actual.alpha_write==old.alpha_write,"compare override preserves source writes/blend/masks");}
    check(!scene.set_depth_compare(8),"invalid comparison refused");draw();check(backend.calls[0].depth_compare==7,"invalid comparison does not install unsupported state");scene.clear_depth_compare();draw();for(unsigned i=0;i<3;++i)check(backend.calls[i].depth_compare==baseline[i].depth_compare&&backend.calls[i].depth_write==baseline[i].depth_write,"clear restores original per-mesh compare/write");
    std::printf("model depth compare: %u owned checks PASS\n",checks);
}
