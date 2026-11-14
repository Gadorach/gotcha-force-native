// Owned shared-renderer pass ordering, global lights/cache and evaluated origins.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/scene_pass_fixture.hpp"
namespace {
void check(bool v,const std::string& why){if(!v){std::fprintf(stderr,"scene pass GL: %s\n",why.c_str());std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);Window window;window.handle=SDL_CreateWindow("Owned scene pass pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned global model context"},&window,&error),error);unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.viewport_width=camera.viewport_height=64;camera.near_plane=.1f;camera.far_plane=100;camera.fov_y_degrees=60;
    auto sample=[&](std::array<int,3> expected){std::array<u8,4> rgba{};glReadBuffer(GL_BACK);glReadPixels(width/2,height/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());bool ok=true;for(unsigned k=0;k<3;++k)ok&=std::abs(int(rgba[k])-expected[k])<=1;if(!ok)std::fprintf(stderr,"case%u expected[%d,%d,%d] got[%u,%u,%u,%u]\n",cases,expected[0],expected[1],expected[2],rgba[0],rgba[1],rgba[2],rgba[3]);check(ok,"global pass/light pixel");++cases;};
    auto blue=scene_pass_fixture::scene(0x60002001,1),red=scene_pass_fixture::scene(0x60002001,-1);for(auto& m:blue.meshes){m.material_colour[0]=m.material_colour[1]=0;m.material_colour[2]=255;}for(auto& m:red.meshes){m.material_colour[0]=255;m.material_colour[1]=m.material_colour[2]=0;}
    MenuScene near,far;near.set_interaction(false);far.set_interaction(false);check(near.load_scene(blue,&error)&&far.load_scene(red,&error),error);
    for(unsigned reverse=0;reverse<2;++reverse){render.begin_frame({});render.reset_arena();check(render.begin_model_pass(2),"begin deferred XLU");if(reverse){check(far.draw(render,camera)==1&&near.draw(render,camera)==1,"far/near callbacks");}else check(near.draw(render,camera)==1&&far.draw(render,camera)==1,"near/far callbacks");check(render.end_model_pass(),"end deferred XLU");render.end_frame();sample({64,0,128});}
    // An explicit PE no-blend override does not turn an XLU material into OPA.
    auto no_blend=red;no_blend.meshes[0].blend=false;MenuScene pe;pe.set_interaction(false);check(pe.load_scene(no_blend,&error),error);render.begin_frame({});render.reset_arena();check(render.begin_model_pass(1)&&pe.draw(render,camera)==0&&render.end_model_pass(),"PE state does not alter source pass classification");check(render.begin_model_pass(2)&&pe.draw(render,camera)==1&&render.end_model_pass(),"XLU pass retained");render.end_frame();sample({255,0,0});
    auto raw=scene_pass_fixture::scene(5,0);raw.meshes[0].material_ambient[0]=raw.meshes[0].material_ambient[1]=raw.meshes[0].material_ambient[2]=255;MenuScene model;model.set_interaction(false);check(model.load_scene(raw,&error),error);auto rig=scene_pass_fixture::rig();rig.lights.resize(1);
    for(unsigned colour:{40u,80u}){for(unsigned k=0;k<3;++k)rig.lights[0].colour[k]=u8(colour);check(render.set_scene_lighting(rig),"copied global rig");render.begin_frame({});render.reset_arena();check(model.draw(render,camera)==1,"same model/camera with global light");render.end_frame();sample({int(colour),int(colour),int(colour)});}
    render.clear_scene_lighting();render.begin_frame({});render.reset_arena();check(model.draw(render,camera)==1,"legacy authored context restored");render.end_frame();sample({255,255,255});
    auto spec=scene_pass_fixture::scene(9,0);for(unsigned k=0;k<3;++k){spec.meshes[0].material_colour[k]=0;spec.meshes[0].material_specular[k]=255;}spec.meshes[0].shininess=50;MenuScene evaluated;evaluated.set_interaction(false);check(evaluated.load_scene(spec,&error),error);SceneLighting infinite;infinite.declared=true;infinite.lights.resize(1);infinite.lights[0].flags=9;for(auto& c:infinite.lights[0].colour)c=255;infinite.lights[0].position[0]=infinite.lights[0].position[2]=1;check(render.set_scene_lighting(infinite),"global specular light");
    for(unsigned origin=0;origin<2;++origin){std::vector<std::array<f32,3>> origins(1);origins[0]={origin?10.f:0.f,0,0};check(evaluated.set_pretransformed_geometry_with_origins({spec.meshes[0].vertices},{true},origins),"explicit evaluated original-joint origin");render.begin_frame({});render.reset_arena();check(evaluated.draw(render,camera)==1,"evaluated model shared shader");render.end_frame();sample(origin?std::array<int,3>{255,255,255}:std::array<int,3>{48,48,48});}
    auto bad=infinite;bad.lights[0].flags=2;check(!render.set_scene_lighting(bad),"unported point-light override refused");render.begin_frame({0,0,0,1});render.reset_arena();check(evaluated.draw(render,camera)==0,"no stale lighting fallback");render.end_frame();sample({0,0,255});
    check(glGetError()==GL_NO_ERROR,"no GL error");std::printf("{\"scene_pass_GL_cases\":%u,\"owned_only\":true,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
