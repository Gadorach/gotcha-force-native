#pragma once
#include "engine/gameplay/startup_static_graphics_scene.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace startup_static_graphics_fixture {
inline std::vector<gf::u8> archive(){
    hsd_record_animation_fixture::Fixture f;f.block.resize(0x4000);f.ptr(0x10,0xd00);f.real(0x30c,1);f.word(0x244,0x60002001);
    for(unsigned i=0;i<37;++i){const unsigned root=0x1000+i*0x80,desc=0x2800+i*16;f.ptr(0xd00+i*4,desc);f.ptr(desc,root);std::copy_n(f.block.data()+0x100,64,f.block.data()+root);f.word(root+8,0);f.word(root+4,i==15||i==16?0x20080000:0x10040080);f.ptr(root+16,0x200);f.real(root+44,1000);}
    // Owned XZ artwork; only inverse-view copyright enters the test viewport.
    f.word(0x38c,3);for(unsigned i=0;i<3;++i){const unsigned at=0x483+i*24;const float y=gf::load_be_f32(f.block.data()+at+4);f.real(at+4,0);f.real(at+8,-y);}
    return f.archive();
}
inline gf::StartupStaticGraphicsProfile profile(const gf::HsdRecordAnimationBank& bank){
    gf::StartupStaticGraphicsProfile p;p.loaded=true;p.bank_hash=bank.hash();p.bank_records=gf::u16(bank.records());p.main_camera_hash=0x12345678;p.alpha_start=1;p.alpha_step=gf::bits_to_f32(0xbd888889);p.zero=0;p.screen_position={0,0,-5};p.screen_rotation_x=gf::bits_to_f32(0x3fc90fdb);
    for(unsigned i=0;i<32;++i){const auto r=gf::u16(36-i);const gf::u8 kind=r==15?1:r==16?2:0;p.entries[i]={r,gf::u8(kind?0x44:r<27?0x45:0x15),kind,gf::u32(0x1000+r*0x80)};}return p;
}
inline gf::StartupCameraPoseState camera(const gf::StartupStaticGraphicsProfile& p){gf::StartupCameraPoseState c;c.initialized=true;c.main_binding=p.main_camera_hash;c.source_aspect=1;c.camera.viewport_width=c.camera.viewport_height=64;c.camera.near_plane=.1f;c.camera.far_plane=100;return c;}
} // namespace startup_static_graphics_fixture
