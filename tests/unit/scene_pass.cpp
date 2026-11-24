#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/scene_pass_fixture.hpp"
#include "tests/unit/startup_actor_frame_fixture.hpp"
namespace {
unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"scene pass: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Backend final:public gf::IRenderBackend {
public:
    std::vector<gf::Vertex> vertices;std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}bool upload_vertices(const gf::Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;using namespace scene_pass_fixture;std::string error;auto p=profile();ModelPassQueue queue;std::vector<ModelPassVisit> plan;check(queue.load(p)&&queue.begin(1),"source queue profile");
    check(queue.register_model(1,0x45,true)&&queue.register_model(2,7,true)&&queue.register_model(3,0x24,true)&&queue.register_model(4,0x14,false)&&queue.plan(&plan),"097B4 mask registration");
    const std::array<u32,7> ids={2,1,2,2,3,1,0};check(plan.size()==6,"source six visits, not mesh blend-depth guessed priorities");for(unsigned i=0;i<6;++i)check(plan[i].model==ids[i],"list prepend/LIFO order per queue");
    check(!queue.register_model(2,7,true)&&!queue.ready()&&!queue.plan(&plan)&&plan.empty(),"duplicate native identity refuses and clears plan");
    for(u8 mask:{u8(0x34),u8(0x54),u8(0x64),u8(0x74),u8(0x80)})check(queue.load(p)&&queue.begin(1)&&!queue.register_model(1,mask,true),"source negative table group/unknown flags refuse");
    for(unsigned mask=0;mask<256;++mask){check(queue.load(p)&&queue.begin(1),"all mask controls start fresh");const bool valid=!(mask&0x80)&&(!(mask&0x7c)||p.translucent_groups[mask>>4]>=0);check(queue.register_model(1,u8(mask),true)==valid,"whole source mask/negative-group domain checked");if(valid){check(queue.plan(&plan),"valid source mask plan");std::vector<unsigned> expected;if(mask&1)expected.push_back(0);if(mask&2)expected.push_back(1);if(mask&0x7c)expected.push_back(unsigned(p.translucent_groups[mask>>4])+2);check(plan.size()==expected.size(),"exact memberships, not alpha/blend inferred registration");for(unsigned i=0;i<expected.size();++i)check(plan[i].queue==expected[i]&&plan[i].pass==p.passes[expected[i]],"source queue/pass table mapping");}}
    auto base=rig();StartupSceneLighting lights;check(lights.load(base,p,123,&error),"bound source light state");
    for(unsigned frame=1;frame<=125;++frame){check(lights.step(frame,frame==2,&error),"source colour callback");const auto& c=lights.channels()->at(0);for(const auto& peer:*lights.channels())check(peer.current==c.current&&peer.phase==c.phase,"all four source colour channels coherent");
        if(frame==2)check((c.current[0]>>24)==143&&c.counter==1&&c.phase==0&&(c.current[0]&255)==0,"RGB division-before-multiply/trunc; alpha retained while interpolating");
        if(frame==16)check(c.current==p.target_colours&&c.phase==1,"terminal forward writes full RGBA target");
        if(frame==106)check(c.phase==2&&c.counter==0,"90 hold callbacks then prepare return without sampling it");
        if(frame==121)check(c.phase==3&&c.current[0]==0x96969600,"terminal return writes baseline RGBA");
        if(frame==122)check(!c.active&&c.phase==0,"next callback clears active without resetting retained colour");}
    Camera camera;camera.eye[2]=10;camera.far_plane=100;check(lights.camera_without_fog_contribution(camera),"visible depth range below authored fog start");camera.far_plane=2000000;check(!lights.camera_without_fog_contribution(camera),"cannot silently drop contributing fog");camera.far_plane=100;
    Backend backend;RenderEngine render(backend);check(render.initialize({64,64,false,true,"source scene pass"},nullptr,&error),"render init");MenuScene near,far,edge;near.set_interaction(false);far.set_interaction(false);edge.set_interaction(false);check(near.load_scene(scene(0x60002001,1,10),&error)&&far.load_scene(scene(0x60002001,-1,20),&error)&&edge.load_scene(scene(0x40002001,0,30),&error),"owned source material pass classes");
    render.begin_frame({});render.reset_arena();check(render.begin_model_pass(2)&&near.draw(render,camera)==1&&far.draw(render,camera)==1&&edge.draw(render,camera)==0&&render.end_model_pass(),"explicit XLU pass filters TEXEDGE regardless of blending");render.end_frame();check(backend.calls.size()==2&&backend.vertices[backend.calls[0].vertex_first].r==20&&backend.vertices[backend.calls[1].vertex_first].r==10,"deferred sort across two independent models far->near");
    render.begin_frame({});render.reset_arena();check(render.begin_model_pass(4)&&edge.draw(render,camera)==1&&near.draw(render,camera)==0&&render.end_model_pass(),"independent texture-edge pass");render.end_frame();
    auto lit=scene(5,0);lit.meshes[0].material_ambient[0]=lit.meshes[0].material_ambient[1]=lit.meshes[0].material_ambient[2]=255;MenuScene object;object.set_interaction(false);check(object.load_scene(lit,&error),"shared lighting object");
    auto global=rig();global.lights.resize(1);global.lights[0].colour[0]=global.lights[0].colour[1]=global.lights[0].colour[2]=40;check(render.set_scene_lighting(global),"explicit global ambient");render.begin_frame({});render.reset_arena();check(object.draw(render,camera)==1,"global light draw queued");render.end_frame();check(backend.calls.size()==1&&backend.vertices.at(backend.calls[0].vertex_first).light[0]==40,"global rig reaches submitted vertex lighting");global.lights[0].colour[0]=80;check(render.set_scene_lighting(global),"change copied global light colour");render.begin_frame({});render.reset_arena();check(object.draw(render,camera)==1,"updated light draw queued");render.end_frame();check(backend.calls.size()==1&&backend.vertices.at(backend.calls[0].vertex_first).light[0]==80,"unchanged camera/pose still invalidates projected lighting cache");
    auto invalid=global;invalid.lights[0].flags=2;check(!render.set_scene_lighting(invalid),"unsupported point-light global override refuses");render.begin_frame({});render.reset_arena();check(object.draw(render,camera)==0,"invalid explicit context cannot fall back to stale archive lighting");render.end_frame();render.clear_scene_lighting();
    render.begin_frame({});render.reset_arena();check(render.begin_model_pass(2),"batch guard setup");Vertex vertices[3];auto at=render.append_vertices(vertices,3);check(!render.draw(Primitive::Triangles,at,3)&&!render.end_model_pass(),"untagged generic/UI draw cannot silently enter source model batch");render.end_frame();check(backend.calls.empty(),"failed batch emits no partial prefix");
    MenuScene unsupported;unsupported.set_interaction(false);check(unsupported.load_scene(scene(0x20000001,0),&error),"legacy material inspection entry preserved");render.begin_frame({});render.reset_arena();check(render.begin_model_pass(1)&&unsupported.draw(render,camera)==0&&!render.end_model_pass(),"unsupported source category refuses whole explicit pass, not silent mesh omission");render.end_frame();
    check(lights.load(base,p,123,&error)&&lights.step(1,true,&error)&&lights.step(2,true,&error)&&lights.channels()->at(0).counter==1,"re-request snapshots current colour and resets only source effect clock");
    check(!lights.step(4,false,&error)&&!lights.ready(),"skipped light frame latches refusal");
    auto bytes=read("local/recomp-evidence/native-startup-scene-pass-20261110/source-contract-v2/startup.gfrp");unsigned original=0;if(!bytes.empty()){StartupScenePassProfile actual;check(startup_scene_pass_load({bytes.data(),bytes.size()},&actual,&error),"actual exported pass/light/fog profile");for(unsigned n=0;n<bytes.size();++n){StartupScenePassProfile bad;check(!startup_scene_pass_load({bytes.data(),n},&bad,&error),"truncated profile refuses");}original=1;}
    std::printf("{\"scene_pass_checks\":%u,\"actual_profile\":%u,\"full_scene_join\":false,\"all_passed\":true}\n",checks,original);
}
