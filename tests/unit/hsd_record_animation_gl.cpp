// Owned record-animation -> existing MenuScene -> GL pixel test. No game data.
#include <SDL.h>
#include <GL/gl.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/animated_model_scene.hpp"
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"record animation GL: %s\n",why);std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;
    bool create(const gf::WindowConfig&,std::string*)override{return false;}
    void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}
    void set_title(const std::string&)override{}bool focused()const override{return false;}
    void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}
    void* native_handle()override{return handle;}
};
}
int main(){
    if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
    Window window;window.handle=SDL_CreateWindow("Owned record-animation pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window.handle){SDL_Quit();return 77;}
    auto backend=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*backend);std::string error;
    check(render.initialize({64,64,false,true,"owned record animation"},&window,&error),error.c_str());
    gf::Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=5;camera.target[0]=camera.target[1]=camera.target[2]=0;
    camera.viewport_width=camera.viewport_height=64;camera.near_plane=.1f;camera.far_plane=100;
    unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto pixel=[&](){std::array<gf::u8,4> result{};glReadBuffer(GL_BACK);glReadPixels(int(width/2),int(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,result.data());return result;};
    auto begin=[&](){render.begin_frame({cases,0,0,1});render.reset_arena();render.state()={};};
    std::printf("{\"owned_record_animation_pixels\":[");
    auto run=[&](unsigned channel,float a,float b,unsigned updates,std::array<int,3> expected){
        hsd_record_animation_fixture::Fixture f;f.word(0x144,0x90);f.word(0x184,0x90);f.word(0x244,0x60002001);f.word(0x304,0xff0000ff);
        f.block[0xa0c]=gf::u8(channel);f.stream(0xc00,a,b);auto bytes=f.archive();gf::HsdRecordAnimationBank bank;gf::AnimatedModelScene scene;
        check(bank.load({bytes.data(),bytes.size()},&error)&&scene.load(bank,0,2,0,0,&error)&&scene.request(0,1,&error),error.c_str());scene.set_enabled(true);
        for(unsigned i=0;i<updates;++i)check(scene.advance(1,&error),error.c_str());
        begin();check(scene.draw(render,camera)==1,"one visible owned triangle");render.end_frame();const auto colour=pixel();
        for(unsigned k=0;k<3;++k)check(std::abs(int(colour[k])-expected[k])<=1,"clamped callback/byte alpha reaches shared GL material state");
        std::printf("%s[%u,%u,%u,%u]",cases?",":"",colour[0],colour[1],colour[2],colour[3]);++cases;
        check(!scene.advance(NAN,&error)&&!scene.ready(),"bad update invalidates scene");begin();check(!scene.draw(render,camera),"no failed-pose replay");render.end_frame();check(pixel()[2]==255,"failed scene leaves clear intact");
    };
    run(4,1.5f,.5f,0,{191,0,64});
    run(4,1.5f,.5f,4,{95,0,64});
    run(4,-.5f,-.5f,0,{0,0,64});
    run(10,-.5f,-.5f,0,{255,0,0});
    run(10,.5f,.5f,0,{127,0,128});
    run(10,1.5f,1.5f,0,{0,0,255});
    std::printf("],\"cases\":%u,\"audio_frames\":0,\"all_passed\":true}\n",cases);
    backend->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
