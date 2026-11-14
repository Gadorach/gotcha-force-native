#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "engine/graphics/depth_clear.hpp"
#include "platform/gl/gl_renderer.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"depth clear GL: %s\n",why);std::abort();}}
class Window final:public gf::IWindowBackend {
public:SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);Window window;window.handle=SDL_CreateWindow("Owned depth-clear pixels",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle)return 77;
    auto gpu=gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"depth clear"},&window,&error),"GL initialize");
    auto quad=[&](f32 depth,Tint colour){std::vector<Vertex> v;ScreenDraw::textured_quad(v,0,0,64,64,depth,0,0,1,1,colour);const auto first=render.append_vertices(v.data(),v.size());check(render.draw(Primitive::Triangles,first,u32(v.size())),"quad");};
    auto pixel=[&](std::array<u8,4> expected){std::array<u8,4> actual{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,actual.data());check(actual==expected,"independent RGBA expectation");};
    for(unsigned mode=0;mode<4;++mode){render.begin_frame({});render.reset_arena();render.state()={};render.state().cull_backface=false;render.state().depth_compare=6;Mat4 identity;render.upload_matrix(0,identity.m);quad(.8f,{255,0,0,255});if(mode)check(draw_depth_clear(render,64,64),"source depth-only boundary");if(mode!=1)quad(.2f,{0,0,255,255});if(mode==3)quad(.1f,{0,255,0,255});check(render.end_frame(),"submit");pixel(mode<2?std::array<u8,4>{255,0,0,255}:std::array<u8,4>{0,0,255,255});}
    render.begin_frame({});render.reset_arena();const auto pending=render.pending_draws();check(!draw_depth_clear(render,0,64)&&!draw_depth_clear(render,NAN,64)&&render.pending_draws()==pending,"invalid dimensions cannot append partial clear");check(render.begin_model_pass(2)&&!draw_depth_clear(render,64,64)&&render.end_model_pass()&&render.end_frame(),"clear refuses to interrupt sorted pass");
    check(glGetError()==GL_NO_ERROR,"GL errors");std::printf("depth clear GL: %u owned checks PASS / 4 pixel cases\n",checks);render.shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();return 0;
}
