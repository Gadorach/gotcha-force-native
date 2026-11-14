#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/option_camera_profile.hpp"
#include "engine/assets/menu_layout.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"hsd camera: %s\n",why);std::abort();}}
struct Fixture {
    std::vector<gf::u8> block=std::vector<gf::u8>(0x280);std::vector<unsigned> fields;
    void word(unsigned at,gf::u32 value){gf::store_be32(block.data()+at,value);}
    void real(unsigned at,float value){word(at,gf::f32_to_bits(value));}
    void ptr(unsigned at,unsigned target){word(at,target);fields.push_back(at);}
    void stream(unsigned at,float first,float second){
        for(unsigned i=0;i<3;++i){block[at+i*6]=2;const auto bits=gf::f32_to_bits(i?second:first);
            for(unsigned k=0;k<4;++k)block[at+i*6+1+k]=gf::u8(bits>>(k*8));
            block[at+i*6+5]=10;}
    }
    Fixture(){
        ptr(4,0x20);ptr(0x20,0x40);ptr(0x24,0x90);
        word(0x44,1);ptr(0x58,0xa0);ptr(0x5c,0xb4);
        real(0xac,10);real(0x68,1);real(0x6c,1000);real(0x70,45);real(0x74,4.0f/3);
        ptr(0x90,0xc8);ptr(0xc8,0xe0);ptr(0xcc,0xf0);ptr(0xf0,0x100);
        real(0xe4,20);ptr(0xe8,0x110);real(0x104,20);ptr(0x108,0x130);
        word(0x114,18);block[0x11c]=10;ptr(0x120,0x160);
        word(0x134,18);block[0x13c]=5;ptr(0x140,0x180);
        stream(0x160,45,45);stream(0x180,0,10);
        // Earlier plausible decoy record, NOT the published camera binding.
        std::copy(block.begin()+0x40,block.begin()+0x78,block.begin()+0x200);
        ptr(0x10,0x200);ptr(0x14,0x90);ptr(0x1d0,0x10);ptr(0x218,0xa0);ptr(0x21c,0xb4);
    }
    std::vector<gf::u8> archive(bool omit_eye=false){
        auto reloc=fields;if(omit_eye)reloc.erase(std::remove(reloc.begin(),reloc.end(),0x58),reloc.end());
        std::vector<gf::u8> b(32+block.size()+reloc.size()*4+8+11);
        gf::store_be32(b.data(),gf::u32(b.size()));gf::store_be32(b.data()+4,gf::u32(block.size()));
        gf::store_be32(b.data()+8,gf::u32(reloc.size()));gf::store_be32(b.data()+12,1);
        std::copy(block.begin(),block.end(),b.begin()+32);unsigned at=unsigned(32+block.size());
        for(unsigned field:reloc){gf::store_be32(b.data()+at,field);at+=4;}
        at+=8;const char name[]="scene_data";std::copy(name,name+11,b.begin()+at);return b;
    }
};
std::vector<gf::u8> profile(){
    std::vector<gf::u8> p(40);unsigned at=0;
    for(gf::u32 x:{0x47464f50u,1u,123u,0u,2u,5u,1u,0u,0x40u,0xc8u}){gf::store_be32(p.data()+at,x);at+=4;}
    return p;
}
}
int main(){
    Fixture f;auto bytes=f.archive();gf::HsdCameraClip clip;std::string error;
    check(gf::hsd_camera_clip_load({bytes.data(),bytes.size()},0,&clip,&error),"published camera loads without geometry");
    check(clip.descriptor==0x40&&clip.clip==0xc8,"published camera wins over earlier plausible decoy");
    gf::Camera camera;camera.near_plane=1;camera.far_plane=1000;
    check(gf::hsd_camera_sample(clip,0,&camera,&error)&&camera.eye[2]==10&&camera.eye[0]==0,"zero updates retain authored camera");
    check(gf::hsd_camera_sample(clip,5,&camera,&error)&&std::fabs(camera.eye[0]-5)<1e-5f&&camera.fov_y_degrees==45,"window/base tracks sampled independently");
    auto before=camera;
    check(!gf::hsd_camera_sample(clip,21,&camera,&error)&&camera.eye[0]==before.eye[0],"out-of-range sample publishes no camera");
    check(!gf::hsd_camera_clip_load({bytes.data(),bytes.size()},1,&clip,&error)&&!clip.animation.present,"absent clip refuses");
    auto reject=[&](Fixture fixture,bool omit_eye=false){auto b=fixture.archive(omit_eye);check(!gf::hsd_camera_clip_load({b.data(),b.size()},0,&clip,&error)&&!clip.animation.present,"malformed binding refuses without partial clip");};
    reject(f,true);
    {auto bad=f;bad.word(0x44,2);reject(bad);}
    {auto bad=f;bad.word(0x44,0x00010001);reject(bad);}
    {auto bad=f;bad.real(0x6c,-1);reject(bad);}
    {auto bad=f;bad.real(0x70,NAN);reject(bad);}
    {auto bad=f;bad.word(0xe0,1);reject(bad);}
    {auto bad=f;bad.word(0x13c,0x04000000);reject(bad);}
    {auto bad=f;bad.ptr(0x130,0x130);reject(bad);}
    {auto b=bytes;b.resize(50);check(!gf::hsd_camera_clip_load({b.data(),b.size()},0,&clip,&error),"truncated container refuses");}
    {auto b=bytes;const auto names=b.size()-11;b.resize(b.size()+300,'a');std::fill(b.begin()+names,b.end(),'a');b.back()=0;gf::store_be32(b.data(),gf::u32(b.size()));
        check(!gf::hsd_camera_clip_load({b.data(),b.size()},0,&clip,&error),"oversized symbol name refuses within budget");}
    {
        auto bad=f;bad.block[0x180]=7;auto b=bad.archive();
        check(gf::hsd_camera_clip_load({b.data(),b.size()},0,&clip,&error),"clip metadata may load before unsupported payload sampled");
        check(!gf::hsd_camera_sample(clip,5,&camera,&error)&&camera.eye[0]==before.eye[0],"unknown packed opcode does not publish partial pose");
    }
    {
        auto bad=f;bad.real(0x60,1);auto b=bad.archive();
        check(gf::hsd_camera_clip_load({b.data(),b.size()},0,&clip,&error)&&!gf::hsd_camera_sample(clip,1,&camera,&error),"unsupported roll refuses sampling");
    }
    auto p=profile();gf::Camera base;base.eye[2]=10;base.target[2]=0;base.near_plane=1;base.far_plane=1000;
    gf::OptionCameraProfile result;
    check(gf::option_camera_profile_load({p.data(),p.size()},{bytes.data(),bytes.size()},123,base,&result,&error),"camera profile checks binding, baseline and complete transition domain");
    {auto bad=p;bad[39]^=4;check(!gf::option_camera_profile_load({bad.data(),bad.size()},{bytes.data(),bytes.size()},123,base,&result,&error),"wrong expected clip refuses");}
    {auto bad=base;bad.eye[0]=2;check(!gf::option_camera_profile_load({p.data(),p.size()},{bytes.data(),bytes.size()},123,bad,&result,&error),"wrong main baseline refuses");}
    {auto bad=base;bad.up[0]=1;bad.up[1]=0;check(!gf::option_camera_profile_load({p.data(),p.size()},{bytes.data(),bytes.size()},123,bad,&result,&error),"wrong baseline orientation refuses");}
    bool real=false;std::ifstream af("local/analysis/arc/stff_mdl.arc",std::ios::binary),lf("local/content/main-menu.gfml",std::ios::binary),pf("local/content/main-menu.gfop",std::ios::binary);
    if(af&&lf&&pf){
        std::vector<gf::u8>a((std::istreambuf_iterator<char>(af)),{}),l((std::istreambuf_iterator<char>(lf)),{}),p((std::istreambuf_iterator<char>(pf)),{});
        gf::HsdScene source,layout;gf::Camera main;
        check(gf::hsd_load({a.data(),a.size()},&source,&error)&&gf::menu_layout_load({l.data(),l.size()},{a.data(),a.size()},source,&layout,&main,&error),"real baseline loads");
        check(gf::option_camera_profile_load({p.data(),p.size()},{a.data(),a.size()},gf::menu_asset_hash({a.data(),a.size()}),main,&result,&error),"real published clip matches observed main baseline and all transition samples");
        camera=main;check(gf::hsd_camera_sample(result.camera,result.timing.option_frame,&camera,&error)&&std::fabs(camera.eye[2]-main.eye[2])>100,"real camera reaches distinct TV framing");real=true;
    }
    std::printf("{\"hsd_camera_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");
}
