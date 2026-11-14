#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/assets/hsd_leaf_material.hpp"
#include "tests/unit/hsd_record_texture_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"record textures: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;HsdTextureState value;value.colour_lerp=true;
    check(hsd_texture_callback(12,2,&value)&&value.high==0xffffffffu,"Konst clamp before byte conversion");
    check(hsd_texture_callback(16,.5f,&value)&&value.low==0x7f000000u,"source f32 multiply then truncation");
    check(hsd_texture_callback(17,-2,&value)&&value.low==0x7f000000u,"negative TEV value clamps zero");
    check(hsd_texture_callback(9,2,&value)&&value.blend==1&&hsd_texture_callback(24,-1,&value)&&value.blend==0,"both source blend callbacks clamp");
    const auto before=value;check(!hsd_texture_callback(1,0,&value)&&value.low==before.low,"image callback not relabeled UV");check(!hsd_texture_callback(6,.1f,&value)&&!hsd_texture_callback(20,1,&value)&&!hsd_texture_callback(2,NAN,&value),"unrepresented rotation/TEV1/nonfinite refuse");
    std::array<f32,2> uv{},offset{};check(hsd_texture_uv({-.25f,.5f},{2,0},{2,1},&uv,&offset)&&uv[0]==1&&offset[0]==.25f&&uv[1]==0,"shared inverse texture transform and near-zero scale");
    auto f=hsd_record_texture_fixture::fixture();auto bytes=f.archive();HsdRecordAnimationBank bank;HsdRecordAnimationBinding binding;
    check(bank.load({bytes.data(),bytes.size()},&error),"owned texture record bank");check(!bank.select(0,2,0,0,&binding,&error),"legacy texture refusal retained");
    AnimatedModelScene scene;check(scene.load_textures(bank,0,2,0,0,&error)&&scene.request(0,1,&error),"explicit retained texture player");
    check(scene.pose()->materials[0].has_texture&&scene.pose()->materials[0].uv_offset[0]==.25f,"first requested UV callback consumed");
    check(scene.advance(.5f,&error)&&scene.pose()->materials[0].uv_offset[0]==.28125f,"fractional texture animation reuses FObj clock");
    check(!scene.advance(NAN,&error)&&!scene.ready()&&!scene.pose(),"failed texture update cannot replay prior pose");
    auto colour=hsd_record_texture_fixture::fixture(16,.5f,1,true);bytes=colour.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&scene.load_textures(bank,0,2,0,0,&error)&&scene.request(0,0,&error),"reviewed TEV-colour animation bound by material");
    check(scene.pose()->materials[0].has_texture_colours&&scene.pose()->materials[0].texture_low_colour==0x7f000000u&&scene.pose()->materials[0].texture_high_colour==0xffffffffu,"animated low colour reaches renderer pose");
    auto bad=hsd_record_texture_fixture::fixture();bad.word(0xd04,1);bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!scene.load_textures(bank,0,2,0,0,&error),"foreign texture id rejected");
    bad=hsd_record_texture_fixture::fixture(6,1,1);bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&scene.load_textures(bank,0,2,0,0,&error)&&!scene.request(0,0,&error),"nonzero sampled rotation rejects before draw");
    bad=hsd_record_texture_fixture::fixture(16,1,1);bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!scene.load_textures(bank,0,2,0,0,&error),"colour tracks without reviewed expression refuse");
    bad=hsd_record_texture_fixture::fixture();bad.ptr(0xd0c,0xec0);bytes=bad.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!scene.load_textures(bank,0,2,0,0,&error),"image switching table refuses");
    // New renderer fields cannot enable a different material expression.
    hsd_record_animation_fixture::Fixture plain;bytes=plain.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&scene.load(bank,0,0,0,0,&error)&&scene.request(0,0,&error),"legacy static record still accepted");
    MenuScene renderer;renderer.set_interaction(false);check(renderer.load_scene(*scene.asset(),&error),"owned renderer");auto pose=*scene.pose();pose.materials[0].has_texture=pose.materials[0].has_texture_colours=true;check(!renderer.set_pose(&pose),"forged dynamic TEV on ordinary material refuses");
    std::printf("record textures: %u owned checks PASS\n",checks);
}
