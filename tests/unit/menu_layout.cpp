#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/menu_layout.hpp"
namespace {
void check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"menu layout: %s\n",text);std::abort();}}
void word(std::vector<gf::u8>& b,gf::u32 x){for(int shift=24;shift>=0;shift-=8)b.push_back(gf::u8(x>>shift));}
void real(std::vector<gf::u8>& b,float x){word(b,gf::f32_to_bits(x));}
gf::HsdScene source(){gf::HsdScene s;s.joints.resize(1);s.joints[0].archive_offset=123;s.joints[0].translation[0]=42;gf::HsdMesh m;m.dobj_offset=456;m.vertices.resize(3);m.indices={0,1,2};s.meshes={m};s.mesh_joint={0};return s;}
std::vector<gf::u8> fixture(gf::ByteView archive,unsigned version=2){
    std::vector<gf::u8>b;word(b,0x47464D4C);word(b,version);word(b,gf::menu_asset_hash(archive));word(b,1);
    for(float x:{0.f,0.f,10.f,0.f,0.f,0.f,0.f,1.f,0.f,45.f,1.f,100.f})real(b,x);
    word(b,123);word(b,0x10);word(b,1);
    word(b,456);word(b,1);word(b,0x808080ff);word(b,0x123456ff);real(b,0.25f);
    if(version==3){word(b,7);word(b,5);for(unsigned i=0;i<7;++i){word(b,i);word(b,456);word(b,456);real(b,float(i));real(b,0);real(b,0);real(b,10);}}
    return b;
}
}
int main(){
    const gf::u8 archive[]={1,2,3};gf::ByteView id{archive,3};const auto original=source();auto bytes=fixture(id);gf::HsdScene out;gf::Camera camera;std::string error;unsigned cases=0;
    check(gf::menu_layout_load({bytes.data(),bytes.size()},id,original,&out,&camera,&error),"valid v2");
    check(out.joints[0].translation[0]==42,"authored transforms retained, not snapshot matrices");
    check(out.joints[0].flags==0x10&&out.meshes[0].hidden,"node and DOBJ visibility");
    check(out.meshes[0].material_alpha==0.25f&&out.meshes[0].material_colour[1]==0x34,"material state");cases+=4;
    bytes=fixture(id,3);check(gf::menu_layout_load({bytes.data(),bytes.size()},id,original,&out,&camera,&error)&&out.menu_seals.size()==7,"seven validated seal bindings");++cases;
    auto reject=[&](std::vector<gf::u8>b){gf::HsdScene bad;gf::Camera c;std::string e;check(!gf::menu_layout_load({b.data(),b.size()},id,original,&bad,&c,&e)&&!e.empty()&&bad.joints.empty(),"reject without partial output");++cases;};
    {auto b=bytes;b[0]=0;reject(b);}
    {auto b=bytes;b[11]^=1;reject(b);}
    {auto b=bytes;b.resize(63);reject(b);}
    {auto b=bytes;b[67]=124;reject(b);}
    {auto b=bytes;b[79]=1;reject(b);}
    {auto b=bytes;b.push_back(0);reject(b);}
    for(std::size_t cut=64;cut<bytes.size();cut+=7){auto b=bytes;b.resize(cut);reject(b);}
    // Optional actual private content: no generated data/pixels are committed.
    std::ifstream af("local/analysis/arc/stff_mdl.arc",std::ios::binary),lf("local/content/main-menu.gfml",std::ios::binary);
    bool have=false;
    if(af&&lf){std::vector<gf::u8>a((std::istreambuf_iterator<char>(af)),{}),l((std::istreambuf_iterator<char>(lf)),{});gf::HsdScene s;
        check(gf::hsd_load({a.data(),a.size()},&s,&error),"real stff decodes");
        check(gf::menu_layout_load({l.data(),l.size()},{a.data(),a.size()},s,&out,&camera,&error),"real private layout decodes");
        check(out.meshes.size()==184&&out.menu_seals.size()==7,"real asset/selection bindings");have=true;++cases;
    }
    std::printf("{\"menu_layout_cases\":%u,\"real_content_present\":%s,\"all_passed\":true}\n",cases,have?"true":"false");
}
