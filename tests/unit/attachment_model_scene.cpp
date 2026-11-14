#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "tests/unit/character_model_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"attachment model: %s\n",why);std::abort();}}
std::vector<gf::u8> bytes(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
std::vector<gf::u8> owned(){auto b=character_model_fixture::archive();character_model_fixture::word(b,32+0x30,0x80);character_model_fixture::word(b,32+0x84,0x10050180);return b;}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;auto data=owned();std::string error;AttachmentModelScene scene;Recorder recorder;RenderEngine render(recorder);
    check(render.initialize({640,480,false,true,"owned attachment"},nullptr,&error),"render seam initializes");
    Camera camera;camera.eye[2]=20;camera.target[2]=0;camera.fov_y_degrees=90;camera.far_plane=100;
    check(scene.load({data.data(),data.size()},0,&error)&&scene.asset().meshes.size()==1&&!scene.ready(),"published single-joint rigid record loads without body wrapper and does not publish constructor pose");
    check(scene.asset().scene.joints[0].flags==0x10050180&&(scene.asset().scene.joints[0].flags&0xe00)==0,"TEXGEN100 is not billboard200; immutable original flags kept");
    check(scene.draw(render,camera)==0,"no draw before supplied transform");
    AttachmentBoneTransform transform;transform.valid=true;transform.world.at(0,3)=3;ActorFadeMaterial material{{16,32,64},true};
    check(scene.set_world_fade(transform,material,false,&error)&&scene.ready(),"normal fade-bound explicit matrix accepted");
    check(scene.world_geometry().world_vertices[0][0].position[0]==2,"root authored translation1 is REPLACED, not added to supplied world");
    scene.upload_textures(render);render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==1,"normal rigid/reflection geometry reaches render seam");render.end_frame();
    check(recorder.calls.size()==1&&recorder.calls[0].post_rgb_add==std::array<u8,3>{16,32,64},"shared fade addition survives texture-lighting draw packet");
    check(scene.asset().scene.meshes[0].generated_uv_required&&scene.asset().scene.meshes[0].texture_matrix_required,"source texgen requirements not cleared on immutable asset");
    check(scene.set_world_fade(transform,material,false,&error)&&scene.world_geometry().world_vertices[0][0].position[0]==2,"repeated transforms do not accumulate");
    check(!scene.set_world_fade(transform,material,true,&error)&&!scene.ready()&&scene.draw(render,camera)==0,"alternate style refuses: render-mode changes cannot be replaced with alpha only");
    material.source_class=false;check(!scene.set_world_fade(transform,material,false,&error),"unknown material class refuses");material.source_class=true;
    transform.valid=false;check(!scene.set_world_fade(transform,material,false,&error),"unpublished transform refuses");transform.valid=true;
    transform.world.at(1,1)=0;check(!scene.set_world_fade(transform,material,false,&error)&&!scene.ready(),"singular render normals refuse without numeric-transform fallback");transform.world=Mat4{};
    check(scene.set_world_fade(transform,material,false,&error),"explicit fresh valid transform restores publication");
    const auto generation=scene.instance_binding();auto moved=std::move(scene);check(!scene.ready()&&moved.ready()&&moved.instance_binding()==generation,"moved renderer retains owned data/generation, moved-from cannot render");
    check(moved.load({data.data(),data.size()},0,&error)&&moved.instance_binding()!=generation&&!moved.ready(),"reload creates new generation and invalidates old publication even while old identity is kept alive");
    check(!moved.load({data.data(),data.size()},99,&error)&&!moved.instance_binding()&&!moved.ready(),"bad reload discards old model binding");
    for(u32 flags:{0x10050380u,0x10051180u,0x10052180u,0x10050190u,0x20090180u}){
        auto bad=data;character_model_fixture::word(bad,32+0x84,flags);check(!moved.load({bad.data(),bad.size()},0,&error),"billboard/instance/perspective/hidden/nonopaque paths not admitted");}
    auto multi=character_model_fixture::archive();check(!moved.load({multi.data(),multi.size()},0,&error),"weighted/multi-joint body record requires different owner, no wrapper reuse");
    check(moved.load({data.data(),data.size()},0,&error),"owned fresh load");data.clear();check(moved.set_world_fade(transform,material,false,&error),"renderer owns source data after input lifetime");
    CharacterModelScene body;CharacterModelRenderState old_gate;old_gate.attachments=true;CharacterWorldPose no_world;
    check(!body.set_world_pose(no_world,old_gate,&error),"legacy body attachment gate remains closed");
    const unsigned owned_checks=checks;unsigned records=0,meshes=0,draws=0;
    for(const char* actor:{"g-red","sasuke"}){auto bank=bytes(std::string("local/content/startup-characters/")+actor+"/model.hsd");if(bank.empty())continue;
        const bool red=std::string(actor)=="g-red";for(unsigned choice=0;choice<2;++choice){const unsigned record=red?(choice?9:10):(choice?14:11),expected=red?2:(choice?2:4);
            const bool ok=moved.load({bank.data(),bank.size()},u16(record),&error);if(!ok)std::fprintf(stderr,"%s record%u: %s\n",actor,record,error.c_str());check(ok&&moved.asset().meshes.size()==expected,"actual published attachment record and complete mesh set");
            check((moved.asset().scene.joints[0].flags&0xe00)==0,"actual record callback uses plain view*supplied-world branch");
            transform.world=Mat4{};check(moved.set_world_fade(transform,material,false,&error),"actual rigid/reflection meshes evaluate from supplied root matrix");
            Camera view;view.eye[2]=1000;view.target[2]=0;view.far_plane=4000;view.fov_y_degrees=100;moved.upload_textures(render);render.begin_frame({});render.reset_arena();const auto n=moved.draw(render,view);render.end_frame();
            check(n==expected,"all actual attachment meshes emit draws under declared camera, not startup visibility claim");records++;meshes+=expected;draws+=n;
        }
    }
    check(records==0||records==4,"optional original-data records cannot silently claim partial four-record coverage");
    std::printf("{\"attachment_model_checks\":%u,\"owned_checks\":%u,\"original_records\":%u,\"original_meshes\":%u,\"declared_camera_draws\":%u,\"actual_constructor_child_or_visible_startup\":false,\"all_passed\":true}\n",checks,owned_checks,records,meshes,draws);
}
