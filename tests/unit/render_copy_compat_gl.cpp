// NEW legacy-interface GL witness. Built against retained before/current code;
// no framebuffer-copy call, feedback owner, old test main or original assets.
#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <fstream>
#include "platform/gl/gl_renderer.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace {
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);Window window;window.handle=SDL_CreateWindow("Owned legacy GL witness",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*gpu);std::string error;if(!render.initialize({16,16,false,true,"new witness"},&window,&error))return 3;unsigned width=0,height=0;window.size(&width,&height);
    for(unsigned frame=0;frame<4;++frame){render.begin_frame({});render.reset_arena();std::vector<gf::Vertex> vertices;gf::ScreenDraw::textured_quad(vertices,1,1,15,15,0,0,0,1,1,{gf::u8(50+frame*50),100,200,255});render.state().depth_test=render.state().depth_write=render.state().cull_backface=false;const auto base=render.append_vertices(vertices.data(),vertices.size());if(!render.draw(gf::Primitive::Triangles,base,vertices.size()))return 3;render.end_frame();std::vector<gf::u8> pixels(std::size_t(width)*height*4);glReadBuffer(GL_BACK);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());gf::u64 hash=1469598103934665603ull;for(auto c:pixels){hash^=c;hash*=1099511628211ull;}std::printf("%u %llu\n",frame,static_cast<unsigned long long>(hash));}
    {std::ifstream maps("/proc/self/maps");std::ofstream save(argv[1]);save<<maps.rdbuf();}
    gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();std::puts("LEGACY_GL_WITNESS_COMPLETED_NO_CAPTURE");std::fflush(stdout);return 0;
}
