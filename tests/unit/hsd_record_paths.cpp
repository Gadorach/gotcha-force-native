#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/animated_model_scene.hpp"
#include "tests/unit/hsd_record_paths_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const std::string& why){++checks;if(!ok){std::fprintf(stderr,"record paths: %s\n",why.c_str());std::abort();}}
gf::ByteView view(const std::vector<gf::u8>& b){return {b.data(),b.size()};}
void owned(){
    using hsd_record_paths_fixture::Fixture;Fixture f;gf::HsdRecordAnimationBank bank;gf::HsdRecordAnimationBinding binding;std::string error;
    auto bytes=f.archive();check(bank.load(view(bytes),&error),error);check(!bank.select(0,3,0,0,&binding,&error)&&!binding.scene(),"legacy selection still refuses PATH");
    check(bank.select_paths(0,3,0,0,&binding,&error),error);check(binding.scene()->joints.size()==3&&binding.scene()->meshes.size()==3&&binding.objects()->at(0).path.valid,"one spline descriptor consumed, all drawable geometry retained");
    gf::HsdRecordAnimationPlayer player;check(player.bind(binding,&error)&&player.request(0,1,&error),"path FIRST_PLAY");check(player.pose()->joints[0].translation[0]==0,"initial point");
    check(player.advance(2,&error)&&player.pose()->joints[0].translation[0]==5,"linear authored arc-length path");check(player.advance(2,&error)&&player.pose()->joints[0].translation[0]==10,"path endpoint");
    gf::AnimatedModelScene scene;check(!scene.load(bank,0,3,0,0,&error)&&!scene.ready(),"legacy model entry retains refusal");check(scene.load_paths(bank,0,3,0,0,&error)&&scene.request(0,1,&error)&&scene.advance(2,&error)&&scene.pose()->joints[0].translation[0]==5,"explicit path owner uses shared scene");
    check(!bank.select_paths(0,0,0,0,&binding,&error),"unconsumed spline nodes refuse static substitute");
    auto reject=[&](Fixture bad,int omit=-1){auto b=bad.archive(omit);gf::HsdRecordAnimationBank other;const bool accepted=other.load(view(b),&error)&&other.select_paths(0,3,0,0,&binding,&error);check(!accepted&&!binding.scene(),"bad or unqualified path binding refuses atomically");};
    reject(f,0x70c);reject(f,0xe10);reject(f,0xe50);reject(f,0xe48);
    {auto b=f;b.word(0xe04,0x4020);reject(b);}
    {auto b=f;b.ptr(0xe08,0x180);reject(b);}
    {auto b=f;b.ptr(0xe3c,0x180);reject(b);}
    {auto b=f;b.ptr(0xe00,0x100);reject(b);}
    {auto b=f;b.block[0xe40]=4;reject(b);}
    {auto b=f;gf::store_be16(b.block.data()+0xe42,1);reject(b);}
    {auto b=f;b.real(0xe4c,0);reject(b);}
    {auto b=f;b.real(0xe60,NAN);reject(b);}
    {auto b=f;b.real(0xe94,0);reject(b);}
    {auto b=f;b.real(0xeb0,INFINITY);reject(b);}
    {auto b=f;b.block[0x74c]=5;reject(b);}
    {auto b=f;b.word(0x70c,0);reject(b);}
    {auto b=f;b.ptr(0x70c,0xf00);b.word(0xf04,0x4000);b.ptr(0xf10,0xe40);reject(b);}
    {auto b=f;b.ptr(0x624,0xee0);auto data=b.archive();check(bank.load(view(data),&error)&&bank.select_paths(0,3,0,0,&binding,&error),"metadata AnimJoint pass-through consumed without inventing a drawable joint");b.ptr(0xee8,0x700);reject(b);}
    {auto b=f;b.ptr(0x824,0xf60);auto data=b.archive();check(bank.load(view(data),&error)&&bank.select_paths(0,3,0,0,&binding,&error),"metadata MatAnimJoint pass-through consumed");b.ptr(0xf68,0x900);reject(b);}
    {auto b=f;b.stream(0x780,-.5f,1.5f);auto data=b.archive();check(bank.load(view(data),&error)&&bank.select_paths(0,3,0,0,&binding,&error)&&player.bind(binding,&error)&&player.request(0,1,&error),"path overshoot fixture");check(player.pose()->joints[0].translation[0]==0&&player.advance(4,&error)&&player.pose()->joints[0].translation[0]==10,"common source arc-parameter endpoint clamp, no fitted path");}
    // A metadata reference by itself does not change a scalar material channel4.
    hsd_record_animation_fixture::Fixture plain;auto p=plain.archive();check(bank.load(view(p),&error)&&bank.select(0,3,0,0,&binding,&error)&&player.bind(binding,&error)&&player.request(0,1,&error)&&player.pose()->materials[0].diffuse[0]==1,"legacy diffuse-red channel4 unchanged");
}
bool original(){
    std::ifstream f("local/recomp-evidence/native-startup-animation-resolution-20261109/recovery-v1/stff_mdl.arc",std::ios::binary);if(!f)return false;std::vector<gf::u8> b((std::istreambuf_iterator<char>(f)),{});gf::HsdRecordAnimationBank bank;std::string error;check(bank.load(view(b),&error),error);gf::HsdRecordAnimationBinding bind;check(!bank.select(55,1,0,0,&bind,&error),"original PATH legacy refusal");check(bank.select_paths(55,1,0,0,&bind,&error),error);check(bind.scene()->joints.size()==8&&bind.objects()->size()==7,"original eight render joints/seven bound path AObjs");
    gf::AnimatedModelScene scene;check(scene.load_paths(bank,55,1,0,0,&error)&&scene.request(0,1,&error),error);for(unsigned i=0;i<100;++i)check(scene.advance(1,&error),error);return true;
}
}
int main(){owned();bool real=original();std::printf("{\"record_path_checks\":%u,\"original_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");}
