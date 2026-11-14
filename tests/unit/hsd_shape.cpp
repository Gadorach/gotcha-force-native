#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/shape_model_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/hsd_shape_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"HSD shape: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;auto f=hsd_shape_fixture::fixture();auto bytes=f.archive();std::string error;HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error),"owned shape bank");HsdRecordAnimationBinding old;check(!bank.select(0,0,0,0,&old,&error),"legacy raw shape refusal");HsdShapeRecord shape;check(shape.load(bank,0,&error)&&shape.shapes()==2,"explicit average shape record");check(shape.original_mesh()->deformation_required,"original raw shape remains flagged");
    std::vector<HsdVertex> pose;float blend=0;check(shape.sample(0,&pose,&blend,&error)&&pose.size()==3&&blend==0&&pose[0].position[0]==-1&&pose[2].source_position_index==2,"shape0 indexed source positions");check(shape.sample(2,&pose,&blend,&error)&&blend==.5f&&pose[0].position[0]==-1.5f&&pose[1].position[1]==1.5f,"adjacent shape interpolation");check(shape.sample(4,&pose,&blend,&error)&&blend==1&&pose[2].position[0]==2,"last shape endpoint");const auto before=pose;check(!shape.sample(5,&pose,&blend,&error)&&pose[0].position[0]==before[0].position[0],"unsupported request preserves output");
    headless::Render backend;headless::Window window;RenderEngine render(backend);check(render.initialize({128,128,false,true,"shape"},&window,&error),"headless renderer");ShapeModelScene scene;check(scene.load(bank,0,render,1,&error),"same MenuScene evaluated geometry owner");Mat4 world;check(scene.sample(2,world,&error),"source shape to world");Camera camera;camera.eye[2]=5;camera.target[2]=0;camera.near_plane=.1f;camera.far_plane=100;render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==1&&render.end_frame(),"shared draw path");world.m[0]=0;check(!scene.sample(2,world,&error),"singular supplied world refuses");render.begin_frame({});render.reset_arena();check(scene.draw(render,camera)==0&&render.end_frame(),"failed pose cannot replay old geometry");
    auto bad=hsd_shape_fixture::fixture();store_be16(bad.block.data()+0xd60,2);bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!shape.load(bank,0,&error),"additive domain not silently treated as average");
    bad=hsd_shape_fixture::fixture();bad.block[0xde0]=255;bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!shape.load(bank,0,&error),"shape index points outside vertex data");
    bad=hsd_shape_fixture::fixture();bad.block[0x485]=3;bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!shape.load(bank,0,&error),"display index exceeds shape point count");
    bad=hsd_shape_fixture::fixture();bad.word(0xd70,1);bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!shape.load(bank,0,&error),"animated normals remain explicit unsupported");
    bad=hsd_shape_fixture::fixture();bad.block[0xf1c]=2;bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!shape.load(bank,0,&error),"average scalar channel contract");
    std::printf("HSD shape: %u owned checks PASS\n",checks);
}
