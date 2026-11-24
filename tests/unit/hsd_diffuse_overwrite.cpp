#include <cstdio>
#include <cstdlib>
#include "tests/unit/hsd_record_texture_fixture.hpp"
#include "engine/assets/hsd_record_animation.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"diffuse overwrite: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto f=hsd_record_texture_fixture::fixture();f.block.resize(0x1200);f.word(0xe40,0x30010);f.ptr(0xe04,0x1000);
    f.word(0x1008,1);f.word(0x100c,5);for(unsigned i=0;i<3;++i)f.real(0x101c+4*i,1);f.block[0x103c]=f.block[0x103d]=1;
    f.word(0x1040,0x30010);f.real(0x1044,1);f.ptr(0x104c,0x1080);f.ptr(0x1080,0x10c0);store_be16(f.block.data()+0x1084,8);store_be16(f.block.data()+0x1086,8);
    std::fill_n(f.block.data()+0xec0,32,u8(0x33));std::fill_n(f.block.data()+0x10c0,32,u8(0xaa));
    auto bytes=f.archive();HsdScene legacy,folded;
    check(hsd_load({bytes.data(),bytes.size()},&legacy,&error)&&!legacy.meshes[0].material_supported&&!legacy.meshes[0].static_diffuse_overwrite,"legacy decoder retains chain refusal");
    check(hsd_load({bytes.data(),bytes.size()},&folded,&error,HsdStaticMaterialPolicy::DiffuseOverwritePair),"explicit static reduction");
    const auto& mesh=folded.meshes[0];check(mesh.material_supported&&mesh.static_diffuse_overwrite&&mesh.texture_uv1&&mesh.texture_colour_mode==3&&mesh.texture_alpha_mode==0&&mesh.texture_weight==1,"last diffuse stage uses its own UV set and exact expression");
    check(folded.textures[mesh.texture].rgba[0]==170&&mesh.material_alpha==.75f,"last texel replaces RGB; material alpha is preserved");
    for(unsigned change=0;change<8;++change){auto bad=f;if(change==0)bad.real(0x1044,.5f);if(change==1)bad.word(0x1040,0x130010);if(change==2)bad.ptr(0x1004,0xe00);if(change==3)bad.real(0x1028,1);if(change==4)bad.word(0x1040,0x30080);if(change==5)bad.word(0x100c,4);if(change==6)bad.word(0x244,0x0400201d);if(change==7)store_be16(bad.block.data()+0xe84,0);
        auto raw=bad.archive();HsdScene refused;check(hsd_load({raw.data(),raw.size()},&refused,&error,HsdStaticMaterialPolicy::DiffuseOverwritePair)&&!refused.meshes[0].material_supported&&!refused.meshes[0].static_diffuse_overwrite,"contributing alpha/partial weight/cycle/transform/placement/UV variants remain unsupported");}
    HsdRecordAnimationBank bank;check(bank.load({bytes.data(),bytes.size()},&error,true,HsdStaticMaterialPolicy::DiffuseOverwritePair),"shared bank retains explicit material policy");
    HsdRecordAnimationBinding binding;check(!bank.select(0,2,0,0,&binding,&error),"material animation cannot invalidate the immutable reduction proof");
    std::printf("HSD diffuse overwrite: %u checks PASS\n",checks);
}
