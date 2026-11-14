#pragma once
#include <cstdlib>
#include "engine/gameplay/startup_reveal_scene.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
#include "tests/unit/character_model_fixture.hpp"
namespace startup_reveal_fixture {
inline gf::u32 hash(const std::vector<gf::u8>& bytes){gf::u32 h=2166136261u;for(auto b:bytes){h^=b;h*=16777619u;}return h;}
inline std::vector<gf::u8> archive(){
    hsd_record_animation_fixture::Fixture f;f.block.resize(0x2000);f.ptr(0x10,0xd00);
    for(unsigned record=0;record<5;++record){const unsigned root=record?0x1000+record*0x100:0x100,desc=0xd40+record*16;f.ptr(0xd00+record*4,desc);f.ptr(desc,root);f.ptr(desc+4,0x50);f.ptr(desc+8,0x60);
        if(!record)continue;
        for(unsigned j=0;j<3;++j){const unsigned at=root+j*64;std::copy_n(f.block.data()+0x100+j*64,64,f.block.data()+at);if(j<2)f.ptr(at+8,at+64);f.ptr(at+16,0x200+j*16);}
    }
    return f.archive();
}
inline std::vector<gf::u8> profile(const gf::HsdRecordAnimationBank& bank,gf::u32 model_hash){
    std::vector<gf::u8> p(64);gf::store_be32(p.data(),0x47465256);gf::store_be16(p.data()+4,1);gf::store_be16(p.data()+6,gf::u16(bank.records()));gf::store_be32(p.data()+8,64);gf::store_be32(p.data()+12,bank.hash());gf::store_be32(p.data()+16,model_hash);gf::store_be32(p.data()+20,model_hash);
    for(unsigned i=0;i<5;++i){gf::HsdRecordAnimationBinding b;std::string error;if(!bank.select(gf::u16(i),3,0,0,&b,&error))std::abort();gf::store_be32(p.data()+24+i*4,b.scene()->model_roots[0]);}
    gf::store_be32(p.data()+48,gf::f32_to_bits(1));gf::store_be16(p.data()+52,40);gf::store_be16(p.data()+54,120);p[56]=2;p[57]=10;p[58]=1;p[59]=0x44;return p;
}
inline gf::CharacterWorldPose world(const gf::CharacterModelScene& model,float x){
    gf::CharacterWorldPose w;for(const auto& b:model.rig().binding.bones){gf::Mat4 m;m.m[3]=w.matrices.empty()?-x:x;w.matrices.push_back(m);w.joint_offsets.push_back(b.joint_offset);}return w;
}
} // namespace startup_reveal_fixture
