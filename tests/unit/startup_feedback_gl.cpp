// Owned framebuffer pixels, orientation/scaling/crop/order and state restoration.
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/startup_feedback_fixture.hpp"
namespace {
void check(bool ok,const std::string& why){if(!ok){std::fprintf(stderr,"feedback GL: %s\n",why.c_str());std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}
    void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);Window window;window.handle=SDL_CreateWindow("Owned feedback pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle){SDL_Quit();return 77;}
    auto gpu=gf::gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({16,16,false,true,"owned framebuffer copy"},&window,&error),error);unsigned width=0,height=0;window.size(&width,&height);unsigned cases=0;
    auto quad=[&](float x0,float y0,float x1,float y1,Tint tint,u32 texture=0){std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,x0,y0,x1,y1,0,0,0,1,1,tint);RenderState state;state.depth_test=state.depth_write=state.cull_backface=false;state.position_matrix=kIdentityPositionMatrix;state.texture=texture;render.state()=state;const auto at=render.append_vertices(vertices.data(),vertices.size());check(render.draw(Primitive::Triangles,at,vertices.size()),"owned quad");};
    auto pixel=[&](float x,float y,std::array<int,3> expected){std::array<u8,4> rgba{};glReadBuffer(GL_BACK);glReadPixels(int(x*width/16),int((16-y)*height/16),1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());bool ok=true;for(unsigned k=0;k<3;++k)ok&=std::abs(int(rgba[k])-expected[k])<=1;if(!ok)std::fprintf(stderr,"case%u point%.3g,%.3g expected[%d,%d,%d] actual[%u,%u,%u,%u]\n",cases,double(x),double(y),expected[0],expected[1],expected[2],rgba[0],rgba[1],rgba[2],rgba[3]);check(ok,"captured/composited pixel");++cases;};
    FramebufferCopy copy;copy.texture=77;copy.width=copy.height=copy.reference_width=copy.reference_height=copy.source_width=copy.source_height=16;
    check(!render.copy_framebuffer(copy),"copy outside frame refuses");render.begin_frame({});render.reset_arena();quad(0,0,8,8,{255,0,0,255});quad(8,0,16,8,{0,255,0,255});quad(0,8,8,16,{0,0,255,255});quad(8,8,16,16,{255,255,255,255});
    // Nondefault pack/read state is restored, not inherited into the copy buffer.
    glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,8);glPixelStorei(GL_PACK_ROW_LENGTH,4);glPixelStorei(GL_PACK_SKIP_ROWS,1);glPixelStorei(GL_PACK_SKIP_PIXELS,2);
    check(render.copy_framebuffer(copy),"checked RGBA8 capture with drawable128->texture16 mapping");GLint state=0;glGetIntegerv(GL_READ_BUFFER,&state);check(state==GL_FRONT,"read buffer restored");glGetIntegerv(GL_PACK_ALIGNMENT,&state);check(state==8,"pack alignment restored");glGetIntegerv(GL_PACK_ROW_LENGTH,&state);check(state==4,"pack row length restored");glGetIntegerv(GL_PACK_SKIP_ROWS,&state);check(state==1,"pack rows restored");glGetIntegerv(GL_PACK_SKIP_PIXELS,&state);check(state==2,"pack pixels restored");glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glReadBuffer(GL_BACK);
    quad(0,0,16,16,{0,0,0,255});quad(0,0,16,16,{255,255,255,255},77);render.end_frame();pixel(4,4,{255,0,0});pixel(12,4,{0,255,0});pixel(4,12,{0,0,255});pixel(12,12,{255,255,255});
    render.begin_frame({});render.reset_arena();quad(0,0,8,8,{255,0,0,255});quad(8,0,16,8,{0,255,0,255});quad(0,8,16,16,{0,0,255,255});auto crop=copy;crop.texture=78;crop.x=8;crop.source_width=crop.source_height=8;crop.width=crop.height=8;check(render.copy_framebuffer(crop),"top-right cropped capture");quad(0,0,16,16,{255,255,255,255},78);render.end_frame();pixel(8,8,{0,255,0});
    // Uploading an ordinary texture to a previously captured handle restores the
    // ordinary top-left texture contract without a stale flip/cache property.
    const std::array<u8,4> magenta={255,0,255,255};TextureDesc tex;tex.width=tex.height=1;tex.pixels=magenta.data();tex.linear=false;check(render.upload_texture(77,tex),"normal upload replaces captured resource");render.begin_frame({});render.reset_arena();quad(0,0,16,16,{255,255,255,255},77);render.end_frame();pixel(8,8,{255,0,255});
    auto raw=startup_feedback_fixture::bytes();StartupFeedbackProfile profile;check(startup_feedback_load({raw.data(),raw.size()},&profile,&error),error);StartupFeedback feedback;check(feedback.load(profile,99,&error),error);StartupDirectorState d;d.config_words=profile.source_words;d.config_value=.5f;StartupFeedbackRender effects;
    std::array<f32,12> nonidentity={1,0,0,200,0,1,0,200,0,0,1,0};check(render.upload_matrix(0,nonidentity.data()),"poison ordinary palette0 to test explicit identity selection");
    for(unsigned frame=1;frame<=4;++frame){d.updates=frame;std::vector<StartupEffect> events;if(frame==1)events.push_back(startup_feedback_fixture::enabled(true));if(frame==4)events.push_back(startup_feedback_fixture::enabled(false));check(feedback.step(d,events,&error),error);render.begin_frame({});render.reset_arena();const Tint colour=frame==1?Tint{255,0,0,255}:frame==2?Tint{0,0,255,255}:Tint{0,255,0,255};quad(0,0,16,16,colour);const auto saved=render.state();check(feedback.render(render,frame,16,16,&effects,&error),error);check(render.state().position_matrix==saved.position_matrix&&render.state().texture==saved.texture,"portable state restored after feedback");render.end_frame();
        if(frame==1){check(effects.captured&&!effects.drawn,"first mode2 frame draws no zero/uninitialized texture");pixel(8,8,{255,0,0});}
        if(frame==2){check(effects.alpha==127,"source truncation not round128");pixel(8,8,{127,0,128});pixel(.25f,.25f,{0,0,255});}
        if(frame==3)pixel(8,8,{63,128,64});
        if(frame==4)pixel(8,8,{0,255,0});
    }
    // An owned pixel-pack buffer blocks readback rather than interpreting a host
    // pointer as a buffer offset. No external/unowned buffer is touched.
    auto gen=reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers"));auto bind=reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer"));auto erase=reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers"));check(gen&&bind&&erase,"owned PBO entry points");GLuint pbo=0;gen(1,&pbo);bind(GL_PIXEL_PACK_BUFFER,pbo);render.begin_frame({});render.reset_arena();check(!render.copy_framebuffer(copy),"bound PBO refuses without unsafe pointer readback");render.end_frame();bind(GL_PIXEL_PACK_BUFFER,0);erase(1,&pbo);
    check(glGetError()==GL_NO_ERROR,"copy/readback leaves no GL error");
    std::printf("{\"startup_feedback_GL_cases\":%u,\"owned_only\":true,\"all_passed\":true}\n",cases);gpu->shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();
}
