// Owned PATH projection plus source alpha-replacement blend/cache pixels.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_intro_props_fixture.hpp"
namespace {
void check(bool ok,const std::string& why){if(!ok){std::fprintf(stderr,"intro GL: %s\n",why.c_str());std::abort();}}
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
    Window window;window.handle=SDL_CreateWindow("Owned intro PATH/alpha",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned intro"},&window,&error),error);
    auto raw=startup_intro_props_fixture::archive(),style=attachment_alternate_fixture::profile();gf::HsdRecordAnimationBank bank;check(bank.load({raw.data(),raw.size()},&error),error);auto pb=startup_intro_props_fixture::profile(bank);gf::StartupIntroPropProfile profile;check(gf::startup_intro_prop_load({pb.data(),pb.size()},&profile,&error),error);gf::StartupIntroPropScene scene;check(scene.load({raw.data(),raw.size()},bank,profile,{style.data(),style.size()},gf::StartupIntroPropMode::DeclaredSceneObservations,&error),error);
    gf::Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=5;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.viewport_width=camera.viewport_height=64;camera.near_plane=.1f;camera.far_plane=100;
    unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto capture=[&](int red,int blue){render.begin_frame({cases,0,0,1});render.reset_arena();scene.draw(render,camera);render.end_frame();gf::u8 rgba[4]={};glReadBuffer(GL_BACK);glReadPixels(int(width/2),int(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);check(std::abs(int(rgba[0])-red)<=1&&rgba[1]==0&&std::abs(int(rgba[2])-blue)<=1,"PATH moved off center; source alpha byte blended over clear");++cases;};
    capture(0,255);for(unsigned tick=0;tick<=12;++tick){gf::StartupDirectorState d;d.updates=tick+1;d.phase=2;check(scene.before_actors(d,1,tick==3?1:0,&error),error);
        if(tick==1){check(scene.path()->pose()->joints[0].translation[0]==2.5f,"authored path position reaches shared hierarchy");capture(229,26);}
        if(tick==2)capture(203,52);
        if(tick==3)capture(0,255);
        if(tick==4)capture(152,103);
        if(tick==9)capture(25,230);
        if(tick==10)capture(0,255);
    }
    gf::StartupDirectorState bad;bad.updates=15;bad.phase=2;check(!scene.before_actors(bad,1,0,&error),"skipped frame refuses");capture(0,255);
    gf::AttachmentAlternateModelScene alternate;check(alternate.load({raw.data(),raw.size()},1,{style.data(),style.size()},&error),error);gf::AttachmentBoneTransform world;world.valid=true;gf::ActorFadeMaterial neutral{{},true};
    for(unsigned pass=0;pass<3;++pass){check(pass==1?alternate.set_world_alpha(world,.9f,&error):alternate.set_world_fade(world,neutral,&error),error);render.begin_frame({cases,0,0,1});render.reset_arena();check(alternate.draw(render,camera)==1,"fixed/dynamic/fixed alternate draw");render.end_frame();gf::u8 rgba[4]={};glReadBuffer(GL_BACK);glReadPixels(int(width/2),int(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);const int alpha=pass==1?229:102;check(std::abs(int(rgba[0])-alpha)<=1&&rgba[1]==0&&std::abs(int(rgba[2])-(255-alpha))<=1,"legacy fixed .4 is restored after explicit source alpha entry");++cases;}
    std::printf("{\"startup_intro_GL_cases\":%u,\"owned_only\":true,\"audio_frames\":0,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
