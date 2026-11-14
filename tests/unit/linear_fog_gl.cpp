#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "platform/gl/gl_renderer.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"linear fog GL: %s\n",why);std::abort();}}
struct Window final:IWindowBackend {SDL_Window* handle=nullptr;bool create(const WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}};
}
int main(){using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);Window w;w.handle=SDL_CreateWindow("Owned linear fog",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!w.handle)return 77;auto backend=gl::create_renderer(nullptr);RenderEngine render(*backend);std::string error;check(render.initialize({64,64,false,true,"fog"},&w,&error),error.c_str());const auto* renderer=reinterpret_cast<const char*>(glGetString(GL_RENDERER));int accelerated=0;SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL,&accelerated);check(renderer&&accelerated&&std::string(renderer).find("llvmpipe")==std::string::npos,"actual hardware context required");unsigned dw=0,dh=0;w.size(&dw,&dh);
    auto pixel=[&](f32 x=.5f){std::array<u8,4> p{};glReadBuffer(GL_BACK);glReadPixels(int(x*dw),int(.45f*dh),1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;};
    auto begin=[&](){render.begin_frame({});render.reset_arena();render.state()={};render.state().cull_backface=false;render.state().depth_test=render.state().depth_write=false;};
    auto quad=[&](f32 left,f32 right,Tint tint){std::vector<Vertex> v;ScreenDraw::textured_quad(v,0,0,64,64,.5f,0,0,1,1,tint);for(auto& p:v)p.inv_w=1.f/(p.x==0?left:right);const auto first=render.append_vertices(v.data(),v.size());check(render.draw(Primitive::Triangles,first,u32(v.size())),"fog quad submission");};
    auto colour=[&](std::array<u8,4> actual,std::array<int,4> expected,const char* why){bool ok=true;for(unsigned i=0;i<4;++i)ok=ok&&std::abs(int(actual[i])-expected[i])<=1;check(ok,why);};
    const LinearFog source{true,5000,6250,{64,166,192}};for(auto depth:{4000.f,5000.f,5625.f,6250.f,7000.f}){begin();render.state().fog=source;quad(depth,depth,{0,0,128,128});check(render.end_frame(),"constant-depth fog frame");f32 f=0;check(linear_fog_factor(source,depth,&f),"CPU source factor");colour(pixel(),{int(std::lround(64*f)),int(std::lround(166*f)),int(std::lround(128*(1-f)+192*f)),128},"source near/mid/far RGB and unchanged alpha");}
    begin();render.state().fog={true,10,20,{0,0,255}};quad(5,40,{255,0,0,255});check(render.end_frame(),"perspective fog interpolation frame");const f32 x=(int(.75f*dw)+.5f)/dw,depth=1.f/((1-x)/5+x/40),f=std::clamp((depth-10)/10,0.f,1.f);colour(pixel(.75f),{int(std::lround(255*(1-f))),0,int(std::lround(255*f)),255},"per-fragment reciprocal clip W, not screen-gradient or clamped vertex-factor interpolation");
    begin();quad(15,15,{0,255,0,255});render.state().fog={true,10,20,{0,0,255}};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;quad(15,15,{255,0,0,128});check(render.end_frame(),"blend fog frame");colour(pixel(),{64,127,64,255},"fog RGB before blend while alpha preserved");
    begin();render.state().post_rgb_add={255,0,0};render.state().fog={true,10,20,{0,0,255}};quad(25,25,{0,0,0,255});check(render.end_frame(),"post colour fog frame");colour(pixel(),{0,0,255,255},"fog after final material RGB hook");
    begin();quad(25,25,{0,255,0,255});render.state().fog={true,10,20,{0,0,255}};render.state().alpha_test=true;quad(25,25,{255,0,0,0});check(render.end_frame(),"alpha-test fog frame");colour(pixel(),{0,255,0,255},"fog never revives alpha-discarded geometry");
    begin();quad(25,25,{255,0,0,255});check(render.end_frame(),"fog disabled frame");colour(pixel(),{255,0,0,255},"per-draw uniforms restore legacy disabled fog");
    std::printf("owned hardware GL linear fog: %u checks PASS; renderer=%s; not GX quantization parity\n",checks,renderer);render.shutdown();SDL_DestroyWindow(w.handle);SDL_Quit();return 0;
}
