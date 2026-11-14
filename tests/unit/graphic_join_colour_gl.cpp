// Owned colour-mobj multiply/add/alpha contract, separate from source assets.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"colour class GL: %s\n",why);std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);Window w;w.handle=SDL_CreateWindow("Owned colour-mobj pixels",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!w.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({64,64,false,true,"colour-mobj contract"},&w,&error),"GL init");unsigned width=0,height=0;w.size(&width,&height);unsigned cases=0;
    const std::array<std::array<u8,3>,4> mul={std::array<u8,3>{255,255,255},{127,127,127},{127,127,127},{255,255,255}};const std::array<std::array<u8,3>,4> add={std::array<u8,3>{0,0,0},{0,0,0},{20,10,5},{0,0,0}};const std::array<std::array<int,4>,4> expected={std::array<int,4>{100,150,200,64},{50,75,100,64},{70,85,105,64},{100,150,200,64}};
    for(unsigned n=0;n<4;++n){render.begin_frame({});render.reset_arena();std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,0,0,64,64,0,0,0,1,1,{100,150,200,64});RenderState state;state.depth_test=state.depth_write=state.cull_backface=state.blend=false;state.post_rgb_multiply=mul[n];state.post_rgb_add=add[n];render.state()=state;auto base=render.append_vertices(vertices.data(),vertices.size());check(render.draw(Primitive::Triangles,base,vertices.size()),"draw");render.end_frame();std::array<u8,4> rgba{};glReadBuffer(GL_BACK);glReadPixels(width/2,height/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());for(unsigned k=0;k<4;++k){if(std::abs(int(rgba[k])-expected[n][k])>1)std::fprintf(stderr,"case%u channel%u actual%u expected%d\n",n,k,unsigned(rgba[k]),expected[n][k]);check(std::abs(int(rgba[k])-expected[n][k])<=1,"completed RGB multiply then add; alpha retained");}++cases;}
    std::printf("{\"colour_class_GL_cases\":%u,\"owned_only\":true,\"all_passed\":true}\n",cases);render.shutdown();SDL_DestroyWindow(w.handle);SDL_Quit();
}
