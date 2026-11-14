// GPU capture versus an independent integer nearest-centre reference, including
// odd scales, crop, resize/reuse, alpha and state restoration. Owned data only.
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstdio>
#include <cstdlib>
#include "platform/gl/gl_renderer.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace {
using namespace gf;
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"GPU copy: %s\n",why);std::abort();}}
struct Window final:IWindowBackend {
    SDL_Window* handle=nullptr;bool create(const WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return true;}void size(unsigned* w,unsigned* h)const override{int x,y;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}
};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO))return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    Window window;window.handle=SDL_CreateWindow("Owned GPU copy regression",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window.handle)return 77;
    auto gpu=gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({128,128,false,true,"copy"},&window,&error),error.c_str());
    unsigned width,height;window.size(&width,&height);check(width==128&&height==128,"owned drawable size");
    std::vector<u8> pixels(128*128*4);for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x){auto p=(y*128+x)*4;pixels[p]=u8(x*17+y*3);pixels[p+1]=u8(y*19+x*7);pixels[p+2]=u8(x^y);pixels[p+3]=u8(x*11+y*13);}
    check(render.upload_texture(1,{128,128,pixels.data(),0,0,false}),"pattern upload");
    auto quad=[&](u32 texture){std::vector<Vertex> v;ScreenDraw::textured_quad(v,0,0,128,128,0,0,0,1,1,{255,255,255,255});render.state()={};render.state().texture=texture;render.state().depth_test=render.state().depth_write=render.state().cull_backface=false;render.state().position_matrix=kIdentityPositionMatrix;auto at=render.append_vertices(v.data(),v.size());check(render.draw(Primitive::Triangles,at,u32(v.size())),"quad");};
    unsigned cases=0;
    for(auto dims:std::array<std::array<u32,2>,9>{{{128,128},{64,64},{97,79},{63,95},{256,192},{1,1},{127,129},{33,37},{96,64}}})for(unsigned crop=0;crop<2;++crop){
        render.begin_frame({});render.reset_arena();quad(1);check(render.end_frame(),"source frame");
        std::vector<u8> source(128*128*4);glReadBuffer(GL_BACK);glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,source.data());
        // Keep source pixels while beginning the copy frame: submit the same
        // owned pattern through the actual barrier, without CPU source upload.
        render.begin_frame({});render.reset_arena();quad(1);
        FramebufferCopy c;c.texture=77;c.reference_width=c.reference_height=128;c.x=crop?7:0;c.y=crop?11:0;c.source_width=crop?103:128;c.source_height=crop?99:128;c.width=dims[0];c.height=dims[1];c.linear=bool(crop);
        glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,8);glPixelStorei(GL_PACK_ROW_LENGTH,7);glPixelStorei(GL_PACK_SKIP_ROWS,2);glPixelStorei(GL_PACK_SKIP_PIXELS,3);
        check(render.copy_framebuffer(c),"GPU capture");GLint value=0;glGetIntegerv(GL_READ_BUFFER,&value);check(value==GL_FRONT,"read state");glGetIntegerv(GL_PACK_ROW_LENGTH,&value);check(value==7,"pack state");
        glPixelStorei(GL_PACK_ALIGNMENT,1);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glReadBuffer(GL_BACK);
        quad(77);check(render.end_frame(),"post-copy draw");
        std::vector<u8> actual(std::size_t(c.width)*c.height*4);glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,actual.data());
        for(u32 y=0;y<c.height;++y)for(u32 x=0;x<c.width;++x){u32 sx=c.x+u32((u64(x)*2+1)*c.source_width/(u64(c.width)*2));u32 sy=c.y+u32((u64(y)*2+1)*c.source_height/(u64(c.height)*2));for(unsigned k=0;k<4;++k){auto expected=source[((127-sy)*128+sx)*4+k];if(actual[(std::size_t(y)*c.width+x)*4+k]!=expected){std::fprintf(stderr,"case%u %ux%u crop%u x%u y%u channel%u got%u expected%u\n",cases,c.width,c.height,crop,x,y,k,actual[(std::size_t(y)*c.width+x)*4+k],expected);check(false,"integer resampling equality");}}}
        ++cases;
    }
    const std::array<u8,4> replacement={10,20,30,40};check(render.upload_texture(77,{1,1,replacement.data(),0,0,false}),"replace copy texture");render.begin_frame({});render.reset_arena();quad(77);check(render.end_frame(),"replacement draw");std::array<u8,4> actual{};glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,actual.data());check(actual==replacement,"replacement contents");
    // Recapture the replaced name: allocation metadata must not retain the
    // dimensions of the old copy after an ordinary upload changed its storage.
    FramebufferCopy recapture;recapture.texture=77;recapture.width=96;recapture.height=64;recapture.reference_width=recapture.reference_height=recapture.source_width=recapture.source_height=128;
    render.begin_frame({});render.reset_arena();quad(1);check(render.copy_framebuffer(recapture),"recapture replaced storage");quad(77);check(render.end_frame(),"recapture frame");GLint tw=0,th=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&tw);glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&th);check(tw==96&&th==64,"recapture reallocates dimensions");
    auto gen=reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers"));auto bind=reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer"));auto erase=reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers"));check(gen&&bind&&erase,"buffer functions");GLuint pbo=0;gen(1,&pbo);
    for(GLenum target:{GL_PIXEL_PACK_BUFFER,GL_PIXEL_UNPACK_BUFFER}){
        render.begin_frame({});render.reset_arena();bind(target,pbo);check(!render.copy_framebuffer(recapture),"foreign transfer buffer refuses");check(!render.end_frame()&&render.frame_failed(),"copy failure latches frame");GLint binding=0;glGetIntegerv(target==GL_PIXEL_PACK_BUFFER?GL_PIXEL_PACK_BUFFER_BINDING:GL_PIXEL_UNPACK_BUFFER_BINDING,&binding);check(GLuint(binding)==pbo,"transfer binding retained");bind(target,0);
    }
    erase(1,&pbo);render.begin_frame({});render.reset_arena();quad(1);check(render.copy_framebuffer(recapture)&&render.end_frame(),"fresh frame after refusal");
    check(glGetError()==GL_NO_ERROR,"GL error");render.shutdown();SDL_DestroyWindow(window.handle);SDL_Quit();std::printf("GPU copy: %u exact RGBA resampling cases + replacement/recapture + pack/unpack refusal/recovery PASS\n",cases);
}
