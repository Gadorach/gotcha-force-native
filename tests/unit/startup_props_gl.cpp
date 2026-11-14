// Owned source prop lifecycle/display gate/rotation through shared GL renderer.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_props_fixture.hpp"
namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"startup props GL: %s\n",why);std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}
    void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}
    void set_title(const std::string&)override{}bool focused()const override{return false;}
    void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}
    void* native_handle()override{return handle;}
};
}
int main(){
    if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    Window window;window.handle=SDL_CreateWindow("Owned startup props test",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned props"},&window,&error),error.c_str());
    auto bytes=startup_props_fixture::archive();gf::HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error),error.c_str());auto pb=startup_props_fixture::profile(bank);gf::StartupPropProfile profile;check(gf::startup_prop_load({pb.data(),pb.size()},&profile,&error),error.c_str());
    gf::StartupPropScene props;check(props.load(bank,profile,gf::StartupPropMode::DeclaredSceneObservations,&error),error.c_str());
    gf::Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=10;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.viewport_width=camera.viewport_height=64;camera.near_plane=.1f;camera.far_plane=100;
    unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto capture=[&](unsigned count,bool white){render.begin_frame({cases,0,0,1});render.reset_arena();check(props.draw(render,camera)==count,"source enabled/gate controls all eight submissions");render.end_frame();gf::u8 pixel[4]={};glReadBuffer(GL_BACK);glReadPixels(int(width/2),int(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);check(pixel[0]==(white?255:0)&&pixel[1]==(white?255:0)&&pixel[2]==255,"shared authored geometry versus clear");++cases;};
    capture(0,false);gf::StartupDirectorState d;d.updates=1;d.phase=1;std::vector<gf::StartupPropSound> cues;check(props.before_actors(d,1,&error)&&props.after_actors(1,0,&cues,&error),error.c_str());capture(0,false);
    check(props.load(bank,profile,gf::StartupPropMode::DeclaredSceneObservations,&error),error.c_str());
    for(unsigned frame=1;frame<=3;++frame){d.updates=frame;d.phase=12;check(props.before_actors(d,2,&error)&&props.after_actors(frame,frame==2?1:0,&cues,&error),error.c_str());check(props.states()->at(4).angles[1]==gf::startup_prop_detail::wrap(-4096*int(frame)),"gear angle persists through settled/display-suppressed callbacks");capture(frame==2?0:8,frame!=2);}
    d.updates=5;check(!props.before_actors(d,2,&error),"skipped frame refuses");capture(0,false);
    std::printf("{\"startup_prop_GL_cases\":%u,\"owned_only\":true,\"audio_frames\":0,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
