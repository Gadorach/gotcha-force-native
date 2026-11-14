#include <SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_scene.hpp"
#include "platform/gl/gl_renderer.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"editor pixels: %s\n",why);std::abort();}}
struct Window final:gf::IWindowBackend {SDL_Window* handle=nullptr;bool create(const gf::WindowConfig&,std::string*)override{return false;}void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}void set_title(const std::string&)override{}bool focused()const override{return false;}void size(unsigned* w,unsigned* h)const override{int x,y;SDL_GL_GetDrawableSize(handle,&x,&y);*w=unsigned(x);*h=unsigned(y);}void* native_handle()override{return handle;}};
}
int main(){
    using namespace gf;if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);Window w;w.handle=SDL_CreateWindow("owned editor pixels",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!w.handle)return 77;auto gpu=gl::create_renderer(nullptr);RenderEngine render(*gpu);std::string error;check(render.initialize({128,128,false,true,"owned editor pixels"},&w,&error),"GL init");
    render.begin_frame({});render.reset_arena();ForceEditorScene::rect(render,0,0,128,128,{30,60,90,255});ForceEditorScene::text(render,"A",30,30,{255,255,255,255},4);check(render.end_frame(),"overlay submission");std::array<u8,4> p{};glReadPixels(5,5,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());check(p[0]==30&&p[1]==60&&p[2]==90,"explicit UI blend factors retain panel colour");std::vector<u8> pixels(128*128*4);glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());unsigned white=0;for(unsigned i=0;i<128*128;++i)white+=pixels[i*4]>240&&pixels[i*4+1]>240&&pixels[i*4+2]>240;check(white>100,"keyboard legend text is visible");
    hsd_record_animation_fixture::Fixture f;auto bytes=f.archive();HsdScene scene;check(hsd_load({bytes.data(),bytes.size()},&scene,&error),"owned scene");scene.meshes.resize(1);scene.mesh_joint.resize(1);auto& m=scene.meshes[0];m.texture=0;m.texture_colour_mode=5;m.texture_alpha_mode=4;m.material_alpha=1;m.material_supported=true;m.texture_stage_inactive=true;scene.textures.resize(1);scene.textures[0].width=scene.textures[0].height=1;scene.textures[0].rgba={255,0,0,255};MenuScene model;model.set_interaction(false);check(model.load_scene(scene,&error),"inactive retained descriptor scene");model.upload_textures(render,1);Camera camera;camera.eye[2]=5;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;camera.viewport_width=camera.viewport_height=128;
    render.begin_frame({});render.reset_arena();check(model.draw(render,camera)==1,"owned triangle");check(render.end_frame(),"inactive-stage submit");glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());check(p[0]>200&&p[1]>200&&p[2]>200,"inactive texture stage does not sample red descriptor");
    std::puts("editor pixels: panel, text, inactive-stage PASS");render.shutdown();SDL_DestroyWindow(w.handle);SDL_Quit();
}
