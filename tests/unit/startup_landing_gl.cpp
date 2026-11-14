// Owned landing world/alpha/lifetime pixels through unchanged shared rendering.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_landing_fixture.hpp"
namespace {
void check(bool ok,const std::string& why){if(!ok){std::fprintf(stderr,"landing GL: %s\n",why.c_str());std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}
    void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);Window window;window.handle=SDL_CreateWindow("Owned landing pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned landings"},&window,&error),error);unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto raw=startup_static_graphics_fixture::archive();HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error),error);auto p=startup_landing_fixture::profile(bank.hash());std::array<StihGrid,3> grids;check(startup_landing_fixture::grids(&grids,&error),error);StartupLandingScene scene;
    auto load=[&](){check(scene.load(bank,p,grids,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,&error),error);};load();
    Camera camera;camera.eye[0]=camera.eye[2]=0;camera.eye[1]=10;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.up[0]=camera.up[1]=0;camera.up[2]=-1;camera.near_plane=.1f;camera.far_plane=100;camera.fov_y_degrees=60;camera.viewport_width=camera.viewport_height=64;
    auto draw=[&](unsigned expected){scene.upload_textures(render);render.begin_frame({cases,0,0,1});render.reset_arena();const auto draws=scene.draw(render,camera);render.end_frame();std::array<u8,4> rgba{};glReadBuffer(GL_BACK);glReadPixels(int(width/2),int(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
        if(rgba[0]!=expected||rgba[1]!=expected||rgba[2]!=255)std::fprintf(stderr,"case%u draws%u expected%u pixel[%u,%u,%u,%u]\n",cases,draws,expected,rgba[0],rgba[1],rgba[2],rgba[3]);
        check(rgba[0]==expected&&rgba[1]==expected&&rgba[2]==255,"original root is REPLACED, curve alpha blends over blue, hidden draws leave clear");++cases;};
    auto request=startup_landing_fixture::request(2);check(scene.step(1,{&request,1},1,0,&error),error);draw(255);
    for(unsigned frame=2;frame<=28;++frame){check(scene.step(frame,{},1,0,&error),error);if(frame==6||frame==11)draw(unsigned(u8(f32(scene.state(0)->alpha*255.f))));if(frame==26){check(scene.state(0)->visible&&scene.state(0)->phase==2&&scene.state(0)->alpha==0,"terminal alpha0 is source-visible before hide");draw(0);}if(frame==27||frame==28)draw(0);}
    check(scene.instances()==0,"natural release");load();request=startup_landing_fixture::request(0);check(scene.step(1,{&request,1},1,0,&error),error);draw(255);check(scene.step(2,{},1,1,&error),error);draw(0);
    load();request.anchor[1]=40;check(scene.step(1,{&request,1},1,0,&error)&&!scene.state(0)->registered,"out-of-distance source rejection is not missing-grid fallback");draw(0);
    request.factor=0;check(!scene.step(2,{&request,1},1,0,&error)&&!scene.ready(),"late failure suppression");draw(0);
    std::printf("{\"landing_GL_cases\":%u,\"owned_only\":true,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
