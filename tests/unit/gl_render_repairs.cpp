// Owned GPU pixel regressions. No proprietary data, no emulator. Requires a
// display; CTest treats exit 77 (no display/context) as an explicit skip.
#include <SDL.h>
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "engine/graphics/render_engine.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include "engine/assets/hsd_leaf_material.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "tests/unit/character_model_fixture.hpp"
#include "platform/gl/gl_renderer.hpp"

namespace {
void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"GL repair: %s\n",what);std::abort();}}
class Window final:public gf::IWindowBackend {
public:
    SDL_Window* handle=nullptr;
    bool create(const gf::WindowConfig&,std::string*)override{return false;}
    void destroy()override{}bool pump_events()override{return true;}bool should_close()const override{return false;}
    void set_title(const std::string&)override{}bool focused()const override{return false;}
    void size(unsigned* w,unsigned* h)const override{int x=0,y=0;SDL_GL_GetDrawableSize(handle,&x,&y);*w=x;*h=y;}
    void* native_handle()override{return handle;}
};
gf::HsdScene foreground_fixture(){
    gf::HsdScene s;s.menu_selection=0;s.menu_motion.present=true;
    s.menu_motion.angle_unit=6.28318530718f/65536;s.menu_motion.selector_joint=900;
    for(unsigned i=0;i<7;++i){
        s.menu_motion.bindings.push_back({i,100+i*3,101+i*3,102+i*3,0});
        for(unsigned j=0;j<3;++j){gf::HsdJoint node;node.archive_offset=100+i*3+j;node.translation[2]=-100;s.joints.push_back(node);}
    }
    gf::HsdJoint selector;selector.archive_offset=900;s.joints.push_back(selector);
    for(unsigned which=0;which<2;++which){
        gf::HsdMesh mesh;mesh.cull_backface=false;mesh.material_colour[0]=which?0:255;mesh.material_colour[1]=which?255:0;mesh.material_colour[2]=0;
        const float width=which?8:12,height=which?4:12;
        for(const auto& xy:std::vector<std::pair<float,float>>{{-width,-height},{width,-height},{width,height},{-width,height}}){
            gf::HsdVertex v;v.position[0]=xy.first;
            if(which)v.position[2]=xy.second;else v.position[1]=xy.second;
            mesh.vertices.push_back(v);
        }
        mesh.indices={0,1,2,0,2,3};s.meshes.push_back(mesh);s.mesh_joint.push_back(which?2:21);
    }
    return s;
}
gf::HsdScene material_fixture(){
    gf::HsdScene s;gf::HsdJoint joint;joint.translation[2]=-100;s.joints.push_back(joint);
    gf::HsdMesh mesh;mesh.cull_backface=false;mesh.texture=0;mesh.texture_colour_mode=4;
    for(const auto& xy:std::vector<std::pair<float,float>>{{-20,-20},{20,-20},{20,20},{-20,20}}){
        gf::HsdVertex v;v.position[0]=xy.first;v.position[1]=xy.second;v.uv[0]=0.5f;v.uv[1]=0.25f;mesh.vertices.push_back(v);
    }
    mesh.indices={0,1,2,0,2,3};s.meshes.push_back(mesh);s.mesh_joint.push_back(0);
    gf::HsdTexture t;t.width=1;t.height=2;t.linear=false;t.rgba={255,255,255,255,0,0,255,255};s.textures.push_back(t);return s;
}
gf::HsdAnimTrack lin(gf::u32 channel,float first,float second){
    gf::HsdAnimTrack t;t.channel=channel;
    for(unsigned i=0;i<3;++i){t.data.push_back(2);const auto bits=gf::f32_to_bits(i?second:first);
        for(unsigned k=0;k<4;++k)t.data.push_back(gf::u8(bits>>(k*8)));
        t.data.push_back(i?8:1);}
    return t;
}
}
int main(){
    if(SDL_Init(SDL_INIT_VIDEO)!=0)return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
    Window w;w.handle=SDL_CreateWindow("Owned GL tests",0,0,128,128,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!w.handle){SDL_Quit();return 77;}
    auto backend=gf::gl::create_renderer(nullptr);gf::RenderEngine render(*backend);std::string error;
    check(render.initialize({64,64,false,true,"test"},&w,&error),error.c_str());
    const auto context=SDL_GL_GetCurrentContext();
    check(!render.initialize({64,64,false,true,"duplicate"},&w,&error),"duplicate initialization refused");
    check(SDL_GL_GetCurrentContext()==context,"no second context created");
    unsigned dw=0,dh=0;w.size(&dw,&dh);
    auto pixel=[&](float x,float y){std::vector<gf::u8> p(4);glReadBuffer(GL_BACK);glReadPixels(int(x*dw),int(y*dh),1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;};
    gf::u8 illumination[4]={255,255,255,255},specular[4]={};bool varying_light=false;
    auto quad=[&](float z,gf::u8 red,gf::u8 green,gf::u8 blue,bool perspective=false,gf::u8 opacity=255){
        std::vector<gf::Vertex> v={{0,0,z,red,green,blue,255,0,0},{64,0,z,red,green,blue,255,1,0},
            {64,64,z,red,green,blue,255,1,1},{0,0,z,red,green,blue,255,0,0},
            {64,64,z,red,green,blue,255,1,1},{0,64,z,red,green,blue,255,0,1}};
        if(perspective)for(auto& p:v)p.inv_w=p.x==0?1.0f:0.5f;
        for(auto& p:v){p.a=opacity;for(unsigned k=0;k<4;++k){p.light[k]=illumination[k];p.specular[k]=specular[k];}}
        if(varying_light)for(auto& p:v){p.light[0]=p.x==0?0:255;p.specular[1]=p.x==0?0:255;}
        auto base=render.append_vertices(v.data(),v.size());check(render.draw(gf::Primitive::Triangles,base,6),"quad draw");
    };
    auto begin=[&](unsigned frame){render.begin_frame({frame,0,0,0});render.reset_arena();render.state()=gf::RenderState{};render.state().cull_backface=false;};
    begin(0);quad(0.8f,0,255,0);render.state().depth_test=false;render.state().depth_write=false;quad(0.8f,0,255,0);render.end_frame();
    check(pixel(0.5f,0.5f)[1]==255,"first green surface");
    begin(1);quad(0.2f,255,0,0);render.end_frame();check(pixel(0.5f,0.5f)[0]==255,"depth clears despite previous UI mask");
    // All four logical corners map across a differently sized drawable.
    check(pixel(0.9f,0.9f)[0]==255,"logical canvas fills scaled drawable");
    begin(2);render.state().cull_backface=true;render.state().front_ccw=false;quad(0.5f,0,0,255);render.end_frame();
    check(pixel(0.5f,0.5f)[2]==255,"explicit CW front face");
    begin(3);render.state().cull_backface=true;render.state().front_ccw=true;quad(0.5f,0,0,255);render.end_frame();
    check(pixel(0.5f,0.5f)[2]==0,"opposite front face culls");
    const gf::u8 cutout[]={255,0,0,0,0,0,255,255};
    check(render.upload_texture(1,{2,1,cutout,0,0,false}),"cutout upload");
    begin(4);quad(0.1f,0,255,0);render.state().texture=1;render.state().texture_colour_mode=5;render.state().texture_alpha_mode=4;
    render.state().alpha_test=true;quad(0.8f,255,255,255);render.end_frame();
    check(pixel(0.25f,0.5f)[1]==255 && pixel(0.75f,0.5f)[2]==255,"cutout comparison and replace modes");
    std::vector<gf::u8> gradient(256*4,255);for(unsigned i=0;i<256;++i){gradient[i*4]=i;gradient[i*4+1]=0;gradient[i*4+2]=0;}
    check(render.upload_texture(2,{256,1,gradient.data(),0,0,true}),"gradient upload");
    begin(5);render.state().texture=2;render.state().texture_colour_mode=5;render.state().texture_alpha_mode=0;
    quad(0.5f,255,255,255,true);render.end_frame();
    const float x=(int(dw/2)+0.5f)/dw;const float expected=256*(0.5f*x/(1-0.5f*x))-0.5f;
    check(std::fabs(float(pixel(0.5f,0.5f)[0])-expected)<3,"perspective-correct UV interpolation");
    begin(6);render.state().texture=2;render.state().texture_colour_mode=6;render.state().texture_alpha_mode=0;quad(0.5f,20,30,40);render.end_frame();
    check(pixel(0.5f,0.5f)[0]==20,"pass combiner preserves material/vertex color");
    const gf::u8 red[]={255,0,0,255},grey[]={128,128,128,255};
    check(render.upload_texture(3,{1,1,red,0,0,false}),"weight fixture upload");
    begin(7);render.state().texture=3;render.state().texture_colour_mode=3;render.state().texture_alpha_mode=0;render.state().texture_weight=0.5f;
    quad(0.5f,0,0,255);render.end_frame();
    auto mixed=pixel(0.5f,0.5f);check(std::abs(int(mixed[0])-128)<2&&std::abs(int(mixed[2])-128)<2,"TOBJ weight blends colors, not opacity");
    check(render.upload_texture(4,{1,1,grey,0,0,false}),"custom expression fixture upload");
    begin(8);render.state().texture=4;render.state().texture_colour_mode=5;render.state().texture_alpha_mode=0;
    render.state().texture_colour_lerp=true;render.state().texture_low_colour=0xFF000000;render.state().texture_high_colour=0xFFFFFFFF;
    quad(0.5f,255,255,255);render.end_frame();
    auto tinted=pixel(0.5f,0.5f);check(tinted[0]==255&&std::abs(int(tinted[1])-128)<2&&std::abs(int(tinted[2])-128)<2,"reviewed custom texture-color expression");
    begin(9);render.state().texture=3;render.state().texture_colour_mode=5;render.state().texture_alpha_mode=0;
    illumination[0]=illumination[1]=illumination[2]=128;specular[1]=64;
    quad(0.5f,255,255,255);render.end_frame();
    auto lit=pixel(0.5f,0.5f);check(std::abs(int(lit[0])-128)<2&&lit[1]==64&&lit[2]==0,"replacement diffuse texture is lit before adding specular");
    begin(10);render.state().texture=3;render.state().texture_colour_mode=5;render.state().texture_alpha_mode=0;
    render.state().texture_after_lighting=true;quad(0.5f,255,255,255);render.end_frame();
    auto ext=pixel(0.5f,0.5f);check(ext[0]==255&&ext[1]==0&&ext[2]==0,"EXT replacement texture is applied after lighting");
    begin(11);specular[0]=255;specular[1]=255;quad(0.5f,255,255,255);render.end_frame();
    auto clamped=pixel(0.5f,0.5f);check(clamped[0]==255&&clamped[1]==255,"specular adds and saturates, never wraps");
    begin(12);specular[0]=specular[1]=0;varying_light=true;quad(0.5f,255,0,0,true);render.end_frame();
    const float lit_expected=255*(0.5f*x/(1-0.5f*x));
    auto interpolated=pixel(0.5f,0.5f);check(std::fabs(float(interpolated[0])-lit_expected)<3&&std::fabs(float(interpolated[1])-lit_expected)<3,"both lighting channels use perspective-correct interpolation");
    begin(13);gf::MenuScene scene;check(scene.load_scene(foreground_fixture(),&error),"owned foreground scene loads");
    gf::Camera camera;camera.viewport_width=camera.viewport_height=64;
    check(scene.draw(render,camera)==2,"selector plus selected caption draws");render.end_frame();
    const auto front=pixel(0.5f,0.5f),surround=pixel(0.65f,0.5f);
    check(front[0]==0&&front[1]==255&&surround[0]==255&&surround[1]==0,"eye-facing selected text is in front of gear selector in actual GL pixels");
    begin(14);scene.set_menu_focus(0,false);check(scene.draw(render,camera)==1,"transition focus suppresses selector draw");render.end_frame();
    const auto focused=pixel(0.5f,0.5f),no_selector=pixel(0.65f,0.5f);
    check(focused[1]==255&&no_selector[0]==0&&no_selector[1]==0,"transition focus keeps active caption but removes selector pixels");
    gf::MenuScene widgets;check(widgets.load_scene(material_fixture(),&error),"owned material scene loads");
    check(widgets.upload_textures(render,20)==1,"owned strip texture uploads");
    gf::HsdLeafMaterialClip clip;clip.end_frame=8;clip.baseline.present=true;clip.baseline.has_texture=true;
    clip.material_tracks={lin(4,0.5f,1),lin(5,0.5f,0.75f),lin(6,0.5f,0.25f)};clip.texture_tracks={lin(3,0,-0.5f)};
    gf::HsdAnimPose pose;pose.joints.resize(1);pose.joints[0].translation[2]=-100;pose.materials.resize(1);
    check(gf::hsd_leaf_material_sample(clip,0,&pose.materials[0],&error)&&widgets.set_pose(&pose),"requested initial pose installed");
    begin(15);check(widgets.draw(render,camera)==1,"initial widget draws");render.end_frame();
    auto initial=pixel(0.5f,0.5f);check(initial[0]==127&&initial[1]==127&&initial[2]==127,"requested time zero gray reaches GL pixels with HSD truncation");
    check(gf::hsd_leaf_material_sample(clip,1,&pose.materials[0],&error),"requested selected material samples");
    const auto selected=pose.materials[0];pose.materials[0].uv_offset[1]=0;
    check(widgets.set_pose(&pose),"selected material pose installed");
    begin(16);widgets.draw(render,camera);render.end_frame();auto yellow=pixel(0.5f,0.5f);
    check(yellow[0]==255&&yellow[1]==191&&yellow[2]==63,"requested diffuse colors replace gray pixels");
    pose.materials[0]=selected;check(widgets.set_pose(&pose),"animated UV pose installed");
    begin(17);widgets.draw(render,camera);render.end_frame();auto shifted=pixel(0.5f,0.5f);
    check(shifted[0]==0&&shifted[1]==0&&shifted[2]==63,"negative T translation samples the next authored texture stripe");
    begin(18);widgets.draw(render,camera);render.end_frame();check(pixel(0.5f,0.5f)==shifted,"cached repeated draw does not accumulate UV shifts");
    check(widgets.set_pose(nullptr),"material pose clears");begin(19);widgets.draw(render,camera);render.end_frame();
    check(pixel(0.5f,0.5f)[0]==255&&pixel(0.5f,0.5f)[1]==255,"clearing pose restores authored UV/material");
    pose.joints[0].matrix_override=true;pose.joints[0].matrix[3]=50;pose.joints[0].matrix[11]=-100;
    check(widgets.set_pose(&pose),"explicit code-driven matrix pose installed");
    begin(20);widgets.draw(render,camera);render.end_frame();
    check(pixel(0.5f,0.5f)[2]==0&&pixel(0.95f,0.5f)[2]==63,"explicit local matrix survives composition and cached projection update");
    varying_light=false;for(auto& c:illumination)c=255;for(auto& c:specular)c=0;
    begin(21);quad(0.1f,0,255,0);render.state().colour_write=false;render.state().alpha_write=false;quad(0.8f,255,255,255);
    render.state().colour_write=true;render.state().alpha_write=true;quad(0.5f,255,0,0);
    render.state().colour_write=false;render.state().alpha_write=false;quad(0.9f,255,255,255);render.end_frame();
    check(pixel(0.5f,0.5f)[1]==255&&pixel(0.5f,0.5f)[0]==0,"depth-only credits occluder blocks farther geometry without painting RGB");
    begin(22);render.end_frame();check(pixel(0.5f,0.5f)[1]==0,"frame clear restores RGB/alpha masks after a depth-only draw");
    begin(23);quad(0.1f,0,255,0);render.state().colour_write=false;render.state().alpha_write=true;quad(0.8f,255,0,0,false,64);render.end_frame();
    auto alpha_only=pixel(0.5f,0.5f);check(alpha_only[1]==255&&alpha_only[0]==0&&alpha_only[3]==64,"alpha-only PE update preserves RGB");
    begin(24);quad(0.1f,0,255,0,false,64);render.state().colour_write=true;render.state().alpha_write=false;quad(0.9f,0,0,255,false,128);render.end_frame();
    check(pixel(0.5f,0.5f)[2]==255&&pixel(0.5f,0.5f)[3]==64,"RGB-only PE update preserves destination alpha");
    auto model_bytes=character_model_fixture::archive(),model_prefix=character_model_fixture::prefix(),model_profile=character_model_fixture::profile();
    gf::CharacterModelScene character;check(character.load({model_bytes.data(),model_bytes.size()},{model_prefix.data(),model_prefix.size()},{model_profile.data(),model_profile.size()},&error),"owned weighted reflection model loads");
    gf::HsdBoneMotionSample model_values;model_values.bones.resize(2);model_values.bones[0].joint_offset=0x40;model_values.bones[1].joint_offset=0x80;
    std::array<gf::CharacterPoseLane,4> model_lanes;model_lanes[0].current=&model_values;gf::CharacterLocalPose model_local;gf::CharacterPoseFrame model_frame;gf::CharacterWorldPose model_world;
    check(gf::character_pose_sample(character.rig(),model_lanes,&model_local,&error)&&gf::character_pose_initial_frame(character.rig(),gf::Mat4{},&model_frame,&error)&&gf::character_pose_direct_world(character.rig(),model_local,model_frame,&model_world,&error),"owned direct matrix pose for GL");
    gf::CharacterModelRenderState model_state;model_state.visible_bones=1;check(character.set_world_pose(model_world,model_state,&error),"weighted owner-only visibility state");
    check(character.upload_textures(render,80)==character.asset().scene.textures.size(),"declared reflection texture references uploaded once");gf::Camera model_camera;model_camera.eye[2]=4;model_camera.target[2]=0;model_camera.far_plane=20;model_camera.fov_y_degrees=60;model_camera.viewport_width=model_camera.viewport_height=64;
    begin(25);check(character.draw(render,model_camera)==1,"weighted reflection triangle submitted");render.end_frame();auto reflected=pixel(.65f,.5f);
    if(!(reflected[2]>240&&reflected[0]<10))std::fprintf(stderr,"weighted reflection pixel RGBA=%u,%u,%u,%u at .65/.5\n",unsigned(reflected[0]),unsigned(reflected[1]),unsigned(reflected[2]),unsigned(reflected[3]));
    check(reflected[2]>240&&reflected[0]<10,"source +normalX reflection samples blue half, not default UV zero/red");
    model_camera.up[1]=-1;begin(26);check(character.draw(render,model_camera)==1,"unchanged pose redraw under rotated camera");render.end_frame();reflected=pixel(.35f,.5f);
    check(reflected[0]>240&&reflected[2]<10,"reflection coordinates regenerate from camera normal basis, not cached old UVs");
    model_state.visible_bones=0;check(character.set_world_pose(model_world,model_state,&error),"source all-hidden node mask");begin(27);check(character.draw(render,model_camera)==0,"all-hidden model emits no geometry");render.end_frame();
    check(pixel(.35f,.5f)[0]==0&&pixel(.35f,.5f)[2]==0,"hidden frame doesn't replay previous weighted geometry");
    gf::HsdScene static_source;check(gf::hsd_load({model_bytes.data(),model_bytes.size()},&static_source,&error),"owned static-reflection scene source");
    static_source.meshes.erase(static_source.meshes.begin());static_source.mesh_joint.erase(static_source.mesh_joint.begin());const auto static_textures=static_source.textures.size();gf::MenuScene static_reflection;
    check(static_reflection.load_scene(std::move(static_source),&error),"ordinary rigid scene explicitly evaluates supported reflection without admitting envelope mesh");
    static_reflection.set_interaction(false);check(static_reflection.upload_textures(render,90)==static_textures,"static reflection texture upload");model_camera.up[1]=1;
    begin(28);check(static_reflection.draw(render,model_camera)==1,"static reflection owner draws");render.end_frame();reflected=pixel(.65f,.5f);
    check(reflected[2]>240&&reflected[0]<10,"static room path also uses source normal reflection, preserving configured menu admission");
    begin(29);render.state().post_rgb_add={64,100,200};quad(.5f,220,40,90,false,64);render.end_frame();auto added=pixel(.5f,.5f);
    check(added[0]==255&&added[1]==140&&added[2]==255&&added[3]==64,"post-material RGB add clamps and preserves source alpha, not opacity fade");
    begin(30);render.state().texture=3;render.state().texture_colour_mode=5;render.state().texture_alpha_mode=0;render.state().post_rgb_add={32,16,8};illumination[0]=illumination[1]=illumination[2]=128;specular[1]=64;quad(.5f,255,255,255);render.end_frame();added=pixel(.5f,.5f);
    check(std::abs(int(added[0])-160)<2&&added[1]==80&&added[2]==8,"fade RGB is added AFTER texture lighting and specular, not lit with vertices");
    for(auto& c:illumination)c=255;
    for(auto& c:specular)c=0;
    begin(31);quad(.5f,20,30,40,false,64);render.end_frame();added=pixel(.5f,.5f);check(added[0]==20&&added[1]==30&&added[2]==40&&added[3]==64,"zero additive state on next draw/frame does not retain old uniform");
    model_state.visible_bones=1;model_state.dynamic_material=true;gf::ActorFadeMaterial fade{{64,32,0},true};check(character.set_world_pose_fade(model_world,model_state,fade,&error),"explicit source fade material installed on weighted reflection model");
    begin(32);render.state().post_rgb_add={1,2,3};check(character.draw(render,model_camera)==1&&render.state().post_rgb_add==std::array<gf::u8,3>{1,2,3},"faded model draw preserves caller render state");render.end_frame();added=pixel(.65f,.5f);
    check(added[0]==64&&added[1]==32&&added[2]==255,"evaluated weighted model reflection and additive material reach actual GL pixels");
    fade.source_class=false;check(!character.set_world_pose_fade(model_world,model_state,fade,&error),"unreviewed dynamic material refuses");begin(33);check(character.draw(render,model_camera)==0,"failed material binding cannot replay last model pose");render.end_frame();check(pixel(.65f,.5f)[0]==0&&pixel(.65f,.5f)[2]==0,"failed source material frame is blank, not stale faded geometry");
    model_state.dynamic_material=false;check(character.set_world_pose(model_world,model_state,&error),"restore legacy base material path");begin(34);character.draw(render,model_camera);render.end_frame();added=pixel(.65f,.5f);check(added[0]<10&&added[1]<10&&added[2]>240,"legacy accepted pose clears prior fade addition");
    check(glGetError()==GL_NO_ERROR,"no GL errors");
    render.shutdown();SDL_DestroyWindow(w.handle);SDL_Quit();
    std::printf("{\"gl_render_repair_cases\":36,\"all_passed\":true}\n");
}
