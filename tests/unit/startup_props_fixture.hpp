#pragma once
#include <cstdlib>
#include "engine/gameplay/startup_props_scene.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace startup_props_fixture {
inline std::vector<gf::u8> archive(){
    hsd_record_animation_fixture::Fixture f;f.block.resize(0x2000);f.ptr(0x10,0xd00);
    for(unsigned i=0;i<8;++i){const unsigned root=0x1000+i*0x100,desc=0xd80+i*16;f.ptr(0xd00+i*4,desc);f.ptr(desc,root);std::copy_n(f.block.data()+0x100,64,f.block.data()+root);f.word(root+8,0);f.word(root+4,0x10050080);f.ptr(root+16,0x200);f.real(root+44,float(i)*.1f);f.real(root+48,0);}
    return f.archive();
}
inline std::vector<gf::u8> profile(const gf::HsdRecordAnimationBank& bank){
    std::vector<gf::u8> p(352);gf::store_be32(p.data(),0x47465050);gf::store_be16(p.data()+4,1);gf::store_be16(p.data()+6,8);gf::store_be32(p.data()+8,352);gf::store_be32(p.data()+12,bank.hash());gf::store_be16(p.data()+16,gf::u16(bank.records()));gf::store_be16(p.data()+18,32);gf::store_be32(p.data()+20,64);gf::store_be32(p.data()+24,320);gf::store_be16(p.data()+28,8);gf::store_be16(p.data()+30,30);gf::store_be32(p.data()+36,0x4622f983);gf::store_be32(p.data()+40,0x38c90fdb);gf::store_be32(p.data()+44,15);gf::store_be32(p.data()+48,0x20000);p[52]=8;p[53]=10;p[55]=2;p[56]=0x28;p[57]=1;
    const gf::s16 delays[]={0,24,16,8,20,20,30,30};const float settle[]={-40,40,-20,20,-10,10,0,0};
    for(unsigned i=0;i<8;++i){const unsigned at=64+i*32;gf::store_be16(p.data()+at,gf::u16(i));gf::store_be32(p.data()+at+4,0x1000+i*0x100);gf::store_be16(p.data()+at+8,gf::u16(delays[i]));gf::store_be16(p.data()+at+10,(i==4||i==6)?379:65535);gf::store_be16(p.data()+at+12,57344);gf::store_be16(p.data()+at+14,i==4?61440:i==6?4096:0);
        gf::store_be_f32(p.data()+at+16,1000);gf::store_be_f32(p.data()+at+20,i<4?-8.f:-40.f);gf::store_be_f32(p.data()+at+24,i<4?-1.5f:-8.f);gf::store_be_f32(p.data()+at+28,i<4?60.f:0.f);gf::store_be_f32(p.data()+320+i*4,settle[i]);}
    return p;
}
} // namespace startup_props_fixture
