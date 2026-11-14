#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_auxiliary_fixture.hpp"
#include "engine/gameplay/startup_auxiliary_scene.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup auxiliary: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class Recorder final:public gf::IRenderBackend {
public:
    std::vector<gf::DrawCall> calls;bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}gf::RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;std::string error;auto model=startup_auxiliary_fixture::model(),bytes=startup_auxiliary_fixture::profile(model);StartupAuxiliaryProfile p;
    check(startup_auxiliary_load({bytes.data(),bytes.size()},&p,&error),"owned descriptor and selected-resource binding");
    for(unsigned at:{0u,4u,6u,8u,20u,24u,25u,26u,27u,28u,29u,30u,48u,128u,152u,156u,158u}){auto bad=bytes;bad[at]^=0x80;StartupAuxiliaryProfile rejected;check(!startup_auxiliary_load({bad.data(),bad.size()},&rejected,&error)&&!rejected.loaded,"malformed identity/count/operand profile refuses");}
    std::array<Mat4,4> bones;bones[3].at(0,3)=10;bones[3].at(1,3)=20;bones[3].at(2,3)=30;StartupAuxiliaryParent parent;parent.model94=0x615;parent.bones=bones;StartupAuxiliaryConstruction construction;
    check(startup_auxiliary_construct(p,0,parent,0,true,0x80,&construction,&error)&&construction.created&&construction.parent708==0x81,"successful40 sets parent bit0 while preserving prior mask");
    check(startup_auxiliary_construct(p,1,parent,0,true,construction.parent708,&construction,&error)&&construction.parent708==0x85,"successful41 sets bit2: source contribution05, NOT blanket assignment");
    check(startup_auxiliary_construct(p,1,parent,5,true,0x80,&construction,&error)&&!construction.created&&construction.parent708==0x80,"scene mode5 skips allocation and parent mutation");
    check(startup_auxiliary_construct(p,1,parent,0,false,0x80,&construction,&error)&&!construction.created&&!construction.parent_write,"failed allocation cannot install parent enable bits");
    parent.kind83=1;check(startup_auxiliary_construct(p,1,parent,0,true,0x80,&construction,&error)&&construction.created&&!construction.parent_write&&construction.parent708==0x80,"other parent kinds create without actor708 mutation");parent.kind83=0;
    check(startup_auxiliary_construct(p,1,parent,0,true,0,&construction,&error),"owned41 construction");StartupAuxiliaryState state=construction.state,next;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.phase==1&&next.counter==0&&next.render_group==1&&!next.matrices_valid,"initializer inline2 matrices/zero counter does NOT run77EC immediately");state=next;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.counter==1&&!next.matrices_valid,"inactive channel increments counter without inventing pose/origin");state=next;parent.active_count=2;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.counter==2&&next.cached_a8==2&&next.previous_a9==0&&next.matrices_valid,"first channel activation computes source non-turn matrices");
    check(next.matrices[0].at(0,3)==12&&next.matrices[0].at(1,3)==23&&next.matrices[0].at(2,3)==34&&next.matrices[1].at(0,3)==8,"bone*T uses local offsets, no actor-scale double application");
    check(std::abs(next.matrices[0].at(2,1)-2)<1e-5f&&std::abs(next.matrices[0].at(1,2)+1)<1e-5f,"counter2 uses pulse1 [3,2,1], post-X rotation then nonuniform scale");state=next;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.previous_a9==2&&std::abs(next.history[0][2]-1)<1e-5f,"history captures NORMALIZED column1 from PREVIOUS world matrix");state=next;parent.active_count=0;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.previous_a9==2&&next.cached_a8==0&&next.matrices[0].at(0,3)==state.matrices[0].at(0,3),"inactive channel retainsA9/history/matrix, does not replay as visible");state=next;parent.active_count=2;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.previous_a9==0,"reactivation breaks old history continuity");state=next;state.counter=32767;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.counter==-32768,"signed16 counter wraps before pulse parity selection");state=next;parent.phase18=2;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.phase==2&&next.render_group==0,"auxiliary77EC retirement stays2 this callback, unlike bank660E8 pre-dispatch retirement");state=next;
    check(next.matrices[1].at(0,0)==3,"hidden phase2 still executes registered41 transform/pulse");bones[3].at(0,3)=100;
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&next.phase==3&&next.allocated,"next handler2 advances3 without freeing early");state=next;
    check(next.matrices[1].at(0,3)==98,"hidden phase3 callback still follows changed parent matrix");parent.bones={};
    check(startup_auxiliary_step(p,1,parent,state,&next,&error)&&!next.allocated,"third handler releases native logical auxiliary state");
    parent.bones=bones;bones[3].at(0,3)=10;
    parent.phase18=1;check(startup_auxiliary_construct(p,0,parent,0,true,0,&construction,&error)&&!startup_auxiliary_step(p,0,parent,construction.state,&next,&error)&&error.find("alignment")!=std::string::npos,"active40 REFUSES missing direction/history consumer, no guessed up-vector solution");
    parent.active_count=0;check(startup_auxiliary_step(p,0,parent,construction.state,&next,&error)&&!next.matrices_valid,"inactive40 can initialize without executing unbound alignment");
    StartupAuxiliaryModelScene scene;const bool loaded=scene.load({model.data(),model.size()},p,&error);if(!loaded)std::fprintf(stderr,"owned scene: %s\n",error.c_str());check(loaded&&!scene.ready(),"owned rigid hierarchy loads but no draw before supplied root");
    Mat4 root;root.at(0,3)=3;check(scene.set_root(root,&error)&&scene.geometry().world_vertices[0][0].position[0]==2&&scene.geometry().world_vertices[1][0].position[0]==3,"root authored999 is REPLACED, child authored1 is preserved");
    Recorder backend;RenderEngine render(backend);render.initialize({640,480,false,true,"owned auxiliary"},nullptr,&error);Camera camera;camera.eye[2]=100;camera.target[2]=0;camera.far_plane=1000;scene.upload_textures(render);render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==2,"complete owned rigid hierarchy emits both meshes");render.end_frame();
    check(backend.calls[0].post_rgb_add==std::array<u8,3>{}&&backend.calls[0].blend&&!backend.calls[0].depth_write,"base shared-scene materials do NOT inherit actor fade wrapper");
    auto moved=std::move(scene);check(!scene.ready()&&moved.ready(),"moving resource invalidates moved-from use");root.at(1,1)=0;check(!moved.set_root(root,&error)&&moved.draw(render,camera)==0,"singular normal matrix refuses without replay");
    const auto owned_checks=checks;auto source_model=read("local/content/startup-auxiliary/efct-51.hsd"),source_profile=read("local/content/startup-auxiliary/startup.gfau");unsigned source_meshes=0;
    if(!source_model.empty()){check(startup_auxiliary_load({source_profile.data(),source_profile.size()},&p,&error),"actual source descriptor profile");const bool ok=scene.load({source_model.data(),source_model.size()},p,&error);if(!ok)std::fprintf(stderr,"source scene: %s\n",error.c_str());check(ok&&scene.asset().scene.joints.size()==4&&scene.asset().meshes.size()==5,"source startup efct slot1/record51 complete4joint5mesh projection");
        parent.active_count=2;parent.phase18=1;check(startup_auxiliary_construct(p,1,parent,0,true,0,&construction,&error)&&startup_auxiliary_step(p,1,parent,construction.state,&next,&error),"actual41 non-turn source operands");scene.upload_textures(render);Camera view;view.eye[2]=1000;view.target[2]=0;view.far_plane=4000;view.fov_y_degrees=100;
        for(unsigned i=0;i<2;++i){check(scene.set_root(next.matrices[i],&error),"source static effect hierarchy evaluates supplied source41 root");render.begin_frame({});render.reset_arena();const auto count=scene.draw(render,view);render.end_frame();check(count==5,"all five source effect meshes draw per declared instance, no hidden mesh substitution");source_meshes+=count;}
    }
    std::printf("{\"startup_auxiliary_checks\":%u,\"owned_checks\":%u,\"source_effect_mesh_draws\":%u,\"actual_parent708_or_sound_consumer_applied\":false,\"active40_alignment_admitted\":false,\"visible_startup_admitted\":false,\"all_passed\":true}\n",checks,owned_checks,source_meshes);
}
