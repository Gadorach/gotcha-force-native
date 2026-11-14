// Owned camera-relative static caption and staged actor reset/exit material pixels.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_static_graphics_fixture.hpp"
#include "tests/unit/startup_actor_frame_fixture.hpp"
namespace {
void check(bool ok,const std::string& why){if(!ok){std::fprintf(stderr,"static/material GL: %s\n",why.c_str());std::abort();}}
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);Window window;window.handle=SDL_CreateWindow("Owned static/material",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"owned static/material"},&window,&error),error);unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto read_pixel=[&](float x,float y){std::array<gf::u8,4> rgba{};glReadBuffer(GL_BACK);glReadPixels(int(x*width),int((1-y)*height),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());return rgba;};
    auto bytes=startup_static_graphics_fixture::archive();gf::HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error),error);auto p=startup_static_graphics_fixture::profile(bank);auto camera=startup_static_graphics_fixture::camera(p);gf::StartupStaticGraphicsScene scene;check(scene.load(bank,p,gf::StartupStaticGraphicsMode::DeclaredSceneObservations,&error),error);
    for(unsigned tick=0;tick<21;++tick){gf::StartupDirectorState d;d.updates=tick+1;d.phase=12;const unsigned mode=tick<3?1:2;if(tick==2){camera.camera.eye[0]=3;camera.camera.eye[1]=2;camera.camera.eye[2]=4;camera.camera.target[0]=-2;camera.camera.target[1]=-1;camera.camera.target[2]=-3;}
        check(scene.before_actors(d,gf::u8(mode),true,&error)&&scene.register_camera(tick+1,camera,0,&error),error);
        if(tick==0||tick==1||tick==2||tick==4||tick==18||tick==19){render.begin_frame({cases,0,0,1});render.reset_arena();scene.draw(render,camera.camera);render.end_frame();const auto rgba=read_pixel(.5f,.5f);const int alpha=tick==0||tick==18||tick==19?0:tick==4?238:255;check(std::abs(int(rgba[0])-alpha)<=1&&std::abs(int(rgba[1])-alpha)<=1&&rgba[2]==255,"copyright stays camera-relative and uses source alpha bytes");++cases;}}
    startup_actor_frame_fixture::Fixture f;check(f.load(&error),error);gf::store_be32(f.fade.data()+24+8,0xffffffff);gf::store_be32(f.fade.data()+24+20,0xffffffff);gf::store_be_f32(f.fade.data()+24+16,99);gf::store_be32(f.fade.data()+52+8,0xffffffff);gf::store_be_f32(f.fade.data()+52+16,30);f.program[92+10]=1;
    gf::StartupActorFrame actor;gf::StartupActorFrameOutput out;check(actor.load(f.assets(),gf::StartupActorFrameMode::IsolatedRequirements,0,&out,&error)&&actor.reset(&out,&error)&&actor.request(0,&out,&error),error);actor.upload_textures(render,1000);gf::StartupActorFrameInput input;input.isolated_external_mutations=input.placement_declared=true;gf::Camera view;view.eye[2]=20;view.target[2]=0;view.near_plane=.1f;view.far_plane=100;view.fov_y_degrees=90;view.viewport_width=view.viewport_height=64;
    auto body_pixel=[&](){render.begin_frame({cases,0,0,1});render.reset_arena();check(actor.draw(render,view)>0,"staged body draw");render.end_frame();const auto& v=actor.model()->world_geometry().world_vertices[0];float center[3]={};for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k)center[k]+=v[i].position[k]/3;float x=0,y=0,scale=0;check(view.project(center,&x,&y,&scale)&&x>0&&x<64&&y>0&&y<64,"owned body triangle sample must be inside test viewport");const auto rgba=read_pixel(x/64,y/64);
        if(rgba[0]!=255||rgba[1]!=255||rgba[2]!=255){std::fprintf(stderr,"body case%u sample(%.9g,%.9g) rgba[%u,%u,%u,%u] material%08x\n",cases,double(x),double(y),rgba[0],rgba[1],rgba[2],rgba[3],actor.state()->material.state().material_rgba);std::vector<gf::u8> pixels;gf::s32 w=0,h=0;if(gf::gl::capture_framebuffer(pixels,&w,&h)){const auto path=std::string("local/recomp-evidence/native-startup-static-material-20261110/body-failure-v2-")+std::to_string(cases)+".ppm";gf::gl::write_ppm(path.c_str(),pixels,w,h);}}
        check(rgba[0]==255&&rgba[1]==255&&rgba[2]==255,"source reset/exit RGB addition reaches weighted body shader");++cases;};
    check(actor.step(input,&out,&error),error);body_pixel();check(actor.step(input,&out,&error),error);unsigned sound242=0;
    for(unsigned i=0;i<10;++i){check(actor.step(input,&out,&error),error);for(const auto& r:out.requirements)sound242+=r.origin==gf::StartupActorRequirementOrigin::Substate&&r.code==gf::u8(gf::StartupActorEffectKind::Sound242Required);if(i==0)body_pixel();}
    check(sound242==1&&!actor.state()->dispatch.state().render_group,"sound242 remains one unfulfilled requirement; source exit hides naturally");render.begin_frame({cases,0,0,1});render.reset_arena();check(!actor.draw(render,view),"hidden body not replayed");render.end_frame();auto cleared=read_pixel(.5f,.5f);check(cleared[0]==0&&cleared[1]==0&&cleared[2]==255,"source hide leaves clear");++cases;
    std::printf("{\"static_material_GL_cases\":%u,\"owned_only\":true,\"audio_frames\":0,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
