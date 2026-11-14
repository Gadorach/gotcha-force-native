// Owned reveal-owner lifetime/display/publication checks through shared GL path.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_reveal_fixture.hpp"
namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"startup reveal GL: %s\n",why);std::abort();}}
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
    Window window;window.handle=SDL_CreateWindow("Owned startup reveal test",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned reveal"},&window,&error),error.c_str());
    gf::HsdRecordAnimationBank bank;auto bytes=startup_reveal_fixture::archive();check(bank.load({bytes.data(),bytes.size()},&error),error.c_str());
    auto model=character_model_fixture::archive(),prefix=character_model_fixture::prefix(),rig=character_model_fixture::profile();std::array<gf::CharacterModelScene,2> actors;
    for(auto& actor:actors)check(actor.load({model.data(),model.size()},{prefix.data(),prefix.size()},{rig.data(),rig.size()},&error),error.c_str());
    auto profileb=startup_reveal_fixture::profile(bank,startup_reveal_fixture::hash(model));gf::StartupRevealProfile profile;check(gf::startup_reveal_load({profileb.data(),profileb.size()},&profile,&error),error.c_str());
    gf::StartupRevealScene scene;check(scene.load(bank,profile,{&actors[0],&actors[1]},gf::StartupRevealMode::DeclaredNativeActorSnapshots,&error),error.c_str());
    gf::Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=10;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.viewport_width=camera.viewport_height=64;camera.near_plane=.1f;camera.far_plane=100;
    const auto world0=startup_reveal_fixture::world(actors[0],0),world1=startup_reveal_fixture::world(actors[1],0);unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto capture=[&](unsigned frame,unsigned expected,bool white){render.begin_frame({frame,0,0,1});render.reset_arena();check(scene.draw(render,camera,{{{&actors[0],nullptr,frame,0},{&actors[1],nullptr,frame,0}}})==expected,"source visibility controls shared submissions");render.end_frame();gf::u8 rgba[4]={};glReadBuffer(GL_BACK);glReadPixels(int(width/2),int(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
        check(rgba[0]==(white?255:0)&&rgba[1]==(white?255:0)&&rgba[2]==255,"visible authored triangle versus untouched blue clear");++cases;};
    for(unsigned frame=1;frame<=47;++frame){gf::StartupDirectorState d;d.updates=frame;d.phase=gf::u8(frame==1?1:frame<43?3:11);const gf::u8 mode=frame<45?1:2;
        check(scene.before_actors(d,mode,true,&error),error.c_str());std::vector<gf::StartupRevealSound> cues;check(scene.after_actors({{{&actors[0],&world0,frame,0},{&actors[1],&world1,frame,0}}},frame==3?1:0,&cues,&error),error.c_str());
        if(frame==1||frame==3||frame==42||frame==45)capture(frame,0,false);
        if(frame==2||frame==4)capture(frame,6,true);
        if(frame==43)capture(frame,9,true);
    }
    gf::StartupDirectorState bad;bad.updates=49;bad.phase=11;check(!scene.before_actors(bad,1,true,&error),"skipped update refuses");capture(49,0,false);
    std::printf("{\"startup_reveal_GL_cases\":%u,\"owned_only\":true,\"audio_frames\":0,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
