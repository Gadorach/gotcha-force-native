// Owned hidden-window GL pixels only. No original assets, startup preview or
// comparison to an original game frame. Exit77 is an explicit unavailable skip.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/attachment_alternate_scene.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
#include "platform/gl/gl_renderer.hpp"
namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"alternate GL: %s\n",why);std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}
    void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=x;*h=y;}void* native_handle()override{return handle;}
};
}
int main(){
    if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    Window window;window.handle=SDL_CreateWindow("Owned alternate pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto backend=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*backend);std::string error;check(render.initialize({64,64,false,true,"alternate pixels"},&window,&error),error.c_str());unsigned dw=0,dh=0;window.size(&dw,&dh);
    auto pixel=[&](){std::array<gf::u8,4> p{};glReadBuffer(GL_BACK);glReadPixels(int(dw/2),int(dh/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;};
    auto quad=[&](float q,gf::u8 r,gf::u8 g,gf::u8 b){render.state()={};render.state().blend=false;render.state().cull_backface=false;const gf::Vertex vertices[]={{0,0,q,r,g,b,255,0,0},{64,0,q,r,g,b,255,0,0},{64,64,q,r,g,b,255,0,0},{0,0,q,r,g,b,255,0,0},{64,64,q,r,g,b,255,0,0},{0,64,q,r,g,b,255,0,0}};const auto base=render.append_vertices(vertices,6);check(render.draw(gf::Primitive::Triangles,base,6),"background/probe draw");};
    auto profile=attachment_alternate_fixture::profile();gf::AttachmentAlternateModelScene scene;gf::AttachmentBoneTransform transform;transform.valid=true;gf::ActorFadeMaterial fade{{0,0,0},true};gf::Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.far_plane=100;camera.fov_y_degrees=60;camera.viewport_width=camera.viewport_height=64;
    auto load=[&](bool pe,bool second_reject=false){auto bank=attachment_alternate_fixture::archive(pe);gf::store_be16(bank.data()+32+0x42c,0); // isolate material pixels from cull-winding tests
        if(second_reject){bank[32+0x600+11]=4;bank[32+0x600+2]=200;}
        check(scene.load({bank.data(),bank.size()},0,{profile.data(),profile.size()},&error)&&scene.set_world_fade(transform,fade,&error),error.c_str());scene.upload_textures(render);};
    auto begin=[&](unsigned frame){render.begin_frame({frame,0,0,0});render.reset_arena();quad(.01f,0,255,0);};
    std::vector<std::array<gf::u8,4>> pixels;
    auto remember=[&](const std::array<gf::u8,4>& p){pixels.push_back(p);};
    load(false);begin(0);check(scene.draw(render,camera)==1,"no-PE alternate draw");render.end_frame();auto p=pixel();remember(p);check(std::abs(int(p[0])-102)<=1&&std::abs(int(p[1])-153)<=1&&p[2]==0&&p[3]==255,"default alpha.4 source-over, not alpha.25*.4; alpha writes preserve background255");
    begin(1);check(scene.draw(render,camera)==1,"depth alternate draw");quad(.025f,0,0,255);render.end_frame();p=pixel();remember(p);check(p[0]==0&&p[1]==0&&p[2]==255,"alternate front q.05 did not write depth; later q.025 probe passes");
    load(true);begin(2);check(scene.draw(render,camera)==1,"explicit PE alternate draw");render.end_frame();p=pixel();remember(p);check(p[0]==255&&p[1]==0&&p[2]==0&&p[3]==102,"explicit PE NONE blending wins over XLU default, alpha-write flag retained");
    load(true,true);begin(3);check(scene.draw(render,camera)==1,"retained compare1 draw");render.end_frame();p=pixel();remember(p);check(p[0]==0&&p[1]==255&&p[2]==0,"ALWAYS compare0 does not erase compare1/operator/ref1 alpha rejection");
    load(false);begin(4);check(scene.draw(render,camera)==1,"no-PE return draw");render.end_frame();p=pixel();remember(p);check(std::abs(int(p[0])-102)<=1&&std::abs(int(p[1])-153)<=1&&p[3]==255,"explicit PE alpha/blend state does not leak into normal fallback copy");
    render.shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();std::fputs("{\"attachment_alternate_GPU_cases\":5,\"pixels\":[",stdout);
    for(unsigned i=0;i<pixels.size();++i){const auto& q=pixels[i];std::printf("%s[%u,%u,%u,%u]",i?",":"",q[0],q[1],q[2],q[3]);}
    std::puts("],\"owned_only\":true,\"all_passed\":true}");
}
