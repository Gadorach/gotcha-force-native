#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/animated_shape_model_scene.hpp"
#include "engine/assets/hsd_shape.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/hsd_shape_material_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"shape material: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;auto f=hsd_shape_material_fixture::fixture();auto data=f.archive();std::string error;HsdRecordAnimationBank bank;check(bank.load({data.data(),data.size()},&error),"owned two-DObj bank");HsdShapeRecord legacy;HsdRecordAnimationBinding plain;check(!legacy.load(bank,0,&error)&&!bank.select_textures(0,2,0,0,&plain,&error),"old shape-only and raw model guards kept");HsdShapeMaterialRecord record;
    check(record.load(bank,0,&error)&&record.meshes()==2&&record.original_mesh(0)->deformation_required&&record.original_mesh(1)->deformation_required,"complete chain consumed; original meshes retain shape flags");std::vector<std::vector<HsdVertex>> geometry;std::vector<f32> blends;
    check(record.sample_shapes(2,&geometry,&blends,&error)&&geometry.size()==2&&blends==std::vector<f32>{.5f,.5f}&&geometry[0][0].position[0]==-1.5f&&geometry[1][2].position[0]==1.5f,"both indexed shapes use common kernel");
    AnimatedShapeModelScene scene;check(scene.load(bank,0,&error)&&scene.sample(2,2,Mat4{},&error),"joined independent shape and material requested clocks");const auto* pose=scene.pose();check(pose&&pose->materials.size()==2&&pose->materials[0].diffuse[0]==127.f/255&&pose->materials[1].alpha==127.f/255,"common MObj callback byte rounding and alpha draw boundary");
    check(scene.sample(4,0,Mat4{},&error)&&scene.pose()->materials[0].diffuse[0]==0&&scene.pose()->materials[1].alpha==1,"new request resets material state independently of shape endpoint");
    headless::Render backend;headless::Window window;RenderEngine render(backend);check(render.initialize({128,128,false,true,"owned combined shape"},&window,&error),"headless renderer");Camera camera;camera.eye[2]=10;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;
    render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==2&&render.end_frame(),"both evaluated meshes use shared draw path");
    check(!scene.sample(5,0,Mat4{},&error)&&!scene.pose(),"unsupported shape request invalidates whole draw");render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==0&&render.end_frame(),"no prior geometry replay");
    check(!scene.sample(0,NAN,Mat4{},&error)&&!scene.pose(),"nonfinite material request refuses");Mat4 singular;singular.m[0]=0;check(!scene.sample(0,0,singular,&error),"singular supplied world refuses");
    f=hsd_shape_material_fixture::fixture();f.ptr(0xf90,0xf90);data=f.archive();check(bank.load({data.data(),data.size()},&error)&&!record.load(bank,0,&error)&&!record.original_mesh(0),"unconsumed/cyclic second shape chain fails whole binding");
    f=hsd_shape_material_fixture::fixture();f.ptr(0x920,0x900);data=f.archive();check(bank.load({data.data(),data.size()},&error)&&!record.load(bank,0,&error),"extra material chain cannot be silently dropped");
    f=hsd_shape_material_fixture::fixture();f.ptr(0x908,0xd00);data=f.archive();check(bank.load({data.data(),data.size()},&error)&&!record.load(bank,0,&error),"texture channels need explicit separate consumer");
    f=hsd_shape_material_fixture::fixture();std::copy_n(f.block.begin()+0xd60,28,f.block.begin()+0xfa0);f.ptr(0x514,0xfa0);f.ptr(0xfa8,0xda0);f.ptr(0xfac,0xfc0);f.ptr(0xfc0,0xde0);f.ptr(0xfc4,0xfe0);f.block[0xfe0]=255;data=f.archive();check(bank.load({data.data(),data.size()},&error)&&!record.load(bank,0,&error),"bad second mesh shape index cannot publish first mesh only");
    std::printf("shape material: %u owned checks PASS\n",checks);
}
