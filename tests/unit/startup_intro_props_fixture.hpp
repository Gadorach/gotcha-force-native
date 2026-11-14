#pragma once
#include "engine/gameplay/startup_intro_props_scene.hpp"
#include "tests/unit/hsd_record_paths_fixture.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace startup_intro_props_fixture {
inline std::vector<gf::u8> archive(){
    hsd_record_paths_fixture::Fixture f;f.block.resize(0x1800);f.ptr(0x10,0xd00);f.ptr(0xd00,0xd30);f.ptr(0xd04,0xd40);f.ptr(0xd30,0x100);f.ptr(0xd34,0x50);f.ptr(0xd38,0x60);f.ptr(0xd40,0xf00);
    std::copy_n(f.block.data()+0x100,64,f.block.data()+0xf00);f.word(0xf04,0x10040000);f.word(0xf08,0);f.ptr(0xf10,0x200);f.word(0x304,0xff0000ff);return f.archive();
}
inline std::vector<gf::u8> profile(const gf::HsdRecordAnimationBank& bank){
    std::vector<gf::u8> p(64);gf::store_be32(p.data(),0x47464954);gf::store_be16(p.data()+4,1);gf::store_be16(p.data()+6,2);gf::store_be32(p.data()+8,64);gf::store_be32(p.data()+12,bank.hash());gf::store_be16(p.data()+16,gf::u16(bank.records()));gf::store_be16(p.data()+20,1);gf::store_be32(p.data()+24,0x100);gf::store_be32(p.data()+28,0xf00);gf::store_be_f32(p.data()+36,1);gf::store_be_f32(p.data()+40,1);gf::store_be_f32(p.data()+44,.1f);p[52]=2;p[53]=1;p[54]=0x28;p[55]=0x47;p[56]=0x44;return p;
}
} // namespace startup_intro_props_fixture
