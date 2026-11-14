// Owned graphics-only owner pixels: no source assets or original camera fitting.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_thruster_fixture.hpp"
namespace {
void check(bool ok,const std::string& why){if(!ok){std::fprintf(stderr,"thruster GL: %s\n",why.c_str());std::abort();}}
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
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    Window window;window.handle=SDL_CreateWindow("Owned thruster pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned jets"},&window,&error),error);unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    startup_thruster_fixture::Fixture f;check(f.load(&error,1),error);
    for(auto& pulse:f.profile.pulse)pulse={1,1,1};
    for(auto& entry:f.profile.entries)for(unsigned n=0;n<2;++n){auto& p=entry.parts[n];p.angle_x=p.angle_z=0;p.position={n?-4.f:4.f,0,0};p.size=1;}
    StartupActorFrame actor;StartupThrusterScene jets;check(f.bind(actor,jets,&error),error);jets.upload_textures(render);
    Camera camera;camera.eye[1]=camera.target[1]=25;camera.eye[2]=50;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=200;camera.fov_y_degrees=90;camera.viewport_width=camera.viewport_height=64;
    auto pixel=[&](float x,float y){std::array<u8,4> rgba{};glReadBuffer(GL_BACK);glReadPixels(int(x*width/64),int((64-y)*height/64),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());return rgba;};
    auto draw=[&](bool visible){render.begin_frame({cases,0,0,1});render.reset_arena();const auto count=jets.draw(actor,render,camera);if(count!=(visible?4u:0u))std::fprintf(stderr,"case%u draws%u expected%u leases[%u,%u] ready%u error%s\n",cases,count,visible?4u:0u,jets.leases()?unsigned(jets.leases()->at(0)):999,jets.leases()?unsigned(jets.leases()->at(2)):999,unsigned(jets.ready()),error.c_str());check(count==(visible?4u:0u),"complete active channel0 meshes only");render.end_frame();
        if(visible)for(unsigned n=0;n<2;++n){const auto& vertices=jets.model(0,n)->geometry().world_vertices[0];float center[3]={};for(unsigned v=0;v<3;++v)for(unsigned k=0;k<3;++k)center[k]+=vertices[v].position[k]/3;float x=0,y=0,scale=0;check(camera.project(center,&x,&y,&scale)&&x>1&&x<63&&y>1&&y<63,"owned triangle centroid inside viewport");const auto rgba=pixel(x,y);const auto alpha=u8(f32(.99f*255.f));if(rgba[0]!=alpha||rgba[1]!=alpha||rgba[2]!=255)std::fprintf(stderr,"case%u copy%u sample%.9g,%.9g rgba[%u,%u,%u,%u]\n",cases,n,double(x),double(y),rgba[0],rgba[1],rgba[2],rgba[3]);check(rgba[0]==alpha&&rgba[1]==alpha&&rgba[2]==255,"both jet copies use owned material alpha .99 through shared shader");}
        else{const auto rgba=pixel(32,32);check(rgba[0]==0&&rgba[1]==0&&rgba[2]==255,"suppressed jets leave clear framebuffer");}++cases;};
    draw(false);StartupActorFrameOutput out;StartupActorFrameInput entry;entry.isolated_external_mutations=entry.placement_declared=true;check(actor.request(0,&out,&error),error);
    for(unsigned frame=1;frame<=10;++frame){check(jets.before_actor(actor,entry,frame,&error)&&actor.step(entry,&out,&error)&&jets.after_actors(actor,frame,frame==2?1:0,&error),error);if(frame<=3||frame==10)draw(frame!=2&&jets.leases()->at(0));}
    check(jets.leases()->at(0)==0&&jets.leases()->at(2)==0,"natural auxiliary expiry, no forced hide");check(actor.request(0,&out,&error),error);draw(false);
    check(jets.ready()==false,"stale parent publication latches refusal");
    std::printf("{\"thruster_GL_cases\":%u,\"owned_only\":true,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
