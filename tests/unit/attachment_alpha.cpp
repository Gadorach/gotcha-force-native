#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/attachment_alternate_scene.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const std::string& why){++checks;if(!ok){std::fprintf(stderr,"attachment alpha: %s\n",why.c_str());std::abort();}}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;std::vector<gf::Vertex> vertices;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();vertices.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
void owned(){
    auto bank=attachment_alternate_fixture::archive(),style=attachment_alternate_fixture::profile();gf::AttachmentAlternateModelScene scene;std::string error;
    check(scene.load({bank.data(),bank.size()},0,{style.data(),style.size()},&error),error);gf::AttachmentBoneTransform world;world.valid=true;
    Recorder backend;gf::RenderEngine render(backend);check(render.initialize({},nullptr,&error),"recorder");gf::Camera camera;camera.eye[0]=camera.eye[1]=0;camera.eye[2]=10;camera.target[0]=camera.target[1]=camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;camera.fov_y_degrees=90;
    auto draw=[&](unsigned alpha){render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==1,"one evaluated alternate mesh");render.end_frame();check(!backend.vertices.empty(),"submitted vertices");for(const auto& vertex:backend.vertices)check(vertex.a==alpha,"source fmuls/truncation alpha byte");};
    check(scene.set_world_alpha(world,.9f,&error)&&scene.material_states()[0].alpha==.9f&&scene.asset().scene.meshes[0].material_alpha==.25f,"alpha replacement, immutable normal asset");draw(229);check(backend.calls[0].post_rgb_add==std::array<gf::u8,3>{},"no forged fade-mobj token or RGB addition");
    check(scene.set_world_alpha(world,.5f,&error),error);draw(127);
    check(!scene.set_world_alpha(world,NAN,&error)&&!scene.ready()&&!scene.draw(render,camera),"invalid alpha invalidates prior draw");check(!scene.set_world_alpha(world,1.1f,&error)&&!scene.set_world_alpha(world,-.1f,&error),"normalized alpha scope");
    gf::ActorFadeMaterial fade{{16,32,64},true};check(scene.set_world_fade(world,fade,&error)&&scene.material_states()[0].alpha==gf::bits_to_f32(0x3ecccccd),"legacy entry restores fixed .4 after new alpha entry");draw(102);check(backend.calls[0].post_rgb_add==fade.rgb,"legacy RGB addition retained");
    check(scene.set_world_alpha(world,1,&error),error);draw(255);check(backend.calls[0].post_rgb_add==std::array<gf::u8,3>{},"new alpha entry clears prior fade RGB");
    auto bad_style=style;gf::store_be_f32(bad_style.data()+48,1);check(!scene.load({bank.data(),bank.size()},0,{bad_style.data(),bad_style.size()},&error),"legacy GFAA fixed-alpha profile still refuses unreviewed value");
    // Direct shared-scene cache: no geometry resubmission between alpha changes.
    gf::HsdScene asset;asset.joints.resize(1);gf::HsdMesh mesh;mesh.vertices.resize(3);const float xy[3][2]={{-1,-1},{0,1},{1,-1}};for(unsigned i=0;i<3;++i){mesh.vertices[i].position[0]=xy[i][0];mesh.vertices[i].position[1]=xy[i][1];}mesh.indices={0,1,2};asset.meshes={mesh,mesh};asset.mesh_joint={0,0};gf::MenuScene renderer;
    check(renderer.load_scene(asset,&error)&&!renderer.set_evaluated_material_alpha({.5f,.5f}),"interaction path cannot use evaluated-material seam");renderer.set_interaction(false);check(renderer.set_evaluated_material_alpha({.4f,.4f}),"explicit identity draw copy");
    render.begin_frame({});render.reset_arena();check(renderer.draw(render,camera)==2,"initial shared scene");render.end_frame();for(const auto& v:backend.vertices)check(v.a==102,"initial cached alpha");
    check(!renderer.set_evaluated_material_alpha({.2f,NAN})&&renderer.scene().meshes[0].material_alpha==.4f,"later invalid alpha preserves all previous material values");check(!renderer.set_evaluated_material_alpha({.2f}),"material shape guard");
    check(renderer.set_evaluated_material_alpha({127/255.f,127/255.f}),"new normalized byte alpha");render.begin_frame({});render.reset_arena();check(renderer.draw(render,camera)==2,"same camera/geometry after alpha update");render.end_frame();for(const auto& v:backend.vertices)check(v.a==127,"material update invalidates projection/shading cache");
    gf::HsdAnimPose pose;pose.joints.resize(1);check(renderer.set_pose(&pose)&&!renderer.set_evaluated_material_alpha({.4f,.4f}),"active authored pose cannot use identity material seam");
}
bool original(){
    std::ifstream f("local/recomp-evidence/native-startup-animation-resolution-20261109/recovery-v1/stff_mdl.arc",std::ios::binary);if(!f)return false;std::vector<gf::u8> bank((std::istreambuf_iterator<char>(f)),{});auto style=attachment_alternate_fixture::profile();gf::AttachmentAlternateModelScene scene;std::string error;check(scene.load({bank.data(),bank.size()},65,{style.data(),style.size()},&error),error);
    const auto& j=scene.asset().scene.joints[0];gf::AttachmentBoneTransform world;world.valid=true;world.world=gf::mat4_from_srt(j.rotation,j.scale,j.translation);float alpha=1;
    for(unsigned i=0;i<11;++i){check(scene.set_world_alpha(world,alpha,&error),error);check(scene.material_states()[0].alpha==alpha,"original record65 raw source alpha state");alpha=gf::f32(alpha-.1f);if(alpha<=0)alpha=0;}
    return true;
}
}
int main(){owned();const bool real=original();std::printf("{\"attachment_alpha_checks\":%u,\"original_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");}
