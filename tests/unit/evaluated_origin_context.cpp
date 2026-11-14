#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/attachment_alternate_scene.hpp"
#include "engine/gameplay/startup_auxiliary_scene.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
#include "tests/unit/startup_auxiliary_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"evaluated origin context: %s\n",why);std::abort();}}
class Backend final:public gf::IRenderBackend {
public:
    std::vector<gf::Vertex> vertices;std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}bool upload_vertices(const gf::Vertex* v,std::size_t n)override{vertices.assign(v,v+n);return true;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}void begin_frame(const gf::FrameDesc&)override{calls.clear();}void submit(const gf::DrawCall& c)override{calls.push_back(c);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;std::string error;auto raw=attachment_alternate_fixture::archive(),style=attachment_alternate_fixture::profile();AttachmentModelScene normal;AttachmentAlternateModelScene alternate;check(normal.load({raw.data(),raw.size()},0,&error)&&alternate.load({raw.data(),raw.size()},0,{style.data(),style.size()},&error),"owned normal/alternate unchanged guard paths");
    AttachmentBoneTransform near,far;near.valid=far.valid=true;near.world.at(2,3)=2;far.world.at(2,3)=-2;ActorFadeMaterial material{{0,0,0},true};check(normal.set_world_fade(near,material,false,&error)&&alternate.set_world_fade(far,material,&error),"accepted explicit evaluated worlds");
    auto model=startup_auxiliary_fixture::model(),profile=startup_auxiliary_fixture::profile(model);StartupAuxiliaryProfile p;StartupAuxiliaryModelScene auxiliary;check(startup_auxiliary_load({profile.data(),profile.size()},&p,&error)&&auxiliary.load({model.data(),model.size()},p,&error),"owned hierarchy for auxiliary");Mat4 root;root.at(2,3)=5;check(auxiliary.set_root(root,&error),"root/child source-world evaluation");
    Backend backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"evaluated-origin compatibility"},nullptr,&error),"render init");SceneLighting lights;lights.declared=true;lights.lights.resize(1);lights.lights[0].flags=4;for(auto& c:lights.lights[0].colour)c=128;check(render.set_scene_lighting(lights),"explicit global lighting context");normal.upload_textures(render,1);alternate.upload_textures(render,10);auxiliary.upload_textures(render,20);Camera camera;camera.eye[2]=100;camera.fov_y_degrees=100;camera.far_plane=1000;
    render.begin_frame({});render.reset_arena();check(render.begin_model_pass(1)&&normal.draw(render,camera)==1&&alternate.draw(render,camera)==0&&auxiliary.draw(render,camera)==0&&render.end_model_pass(),"normal OPA source class, alternate and auxiliary XLU excluded");check(render.begin_model_pass(2)&&auxiliary.draw(render,camera)==2&&alternate.draw(render,camera)==1&&normal.draw(render,camera)==0&&render.end_model_pass(),"all evaluated models use same pass path");render.end_frame();check(backend.calls.size()==4,"one normal +one alternate +two auxiliary mesh draws");
    check(backend.vertices[backend.calls[1].vertex_first].a==102&&backend.vertices[backend.calls[2].vertex_first].a==252,"explicit copied joint origins sort farther alternate before both auxiliary meshes despite callback order");
    auto moved=std::move(alternate);check(!alternate.ready()&&moved.ready(),"origin and generation survive move");render.begin_frame({});render.reset_arena();check(render.begin_model_pass(2)&&moved.draw(render,camera)==1&&render.end_model_pass(),"moved evaluated origin context");render.end_frame();
    auto bad=near;bad.world.at(0,0)=0;check(!normal.set_world_fade(bad,material,false,&error)&&!normal.ready()&&normal.draw(render,camera)==0,"failed evaluated pose does not replay old normal/origin");
    std::printf("{\"evaluated_origin_context_checks\":%u,\"owned_only\":true,\"all_passed\":true}\n",checks);
}
