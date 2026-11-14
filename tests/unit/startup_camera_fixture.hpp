#pragma once
#include "engine/gameplay/startup_camera.hpp"
namespace startup_camera_fixture {
struct Fixture {
    std::vector<gf::u8> block=std::vector<gf::u8>(0x280);std::vector<unsigned> fields;
    void word(unsigned at,gf::u32 v){gf::store_be32(block.data()+at,v);}
    void real(unsigned at,float v){gf::store_be_f32(block.data()+at,v);}
    void ptr(unsigned at,unsigned value){word(at,value);fields.push_back(at);}
    void stream(unsigned at,float a,float b){for(unsigned i=0;i<3;++i){block[at+i*6]=2;const auto v=gf::f32_to_bits(i?b:a);for(unsigned k=0;k<4;++k)block[at+i*6+1+k]=gf::u8(v>>(k*8));block[at+i*6+5]=10;}}
    Fixture(bool special=false){
        ptr(4,0x20);ptr(0x20,0x40);ptr(0x24,0x90);word(0x44,1);ptr(0x58,0xa0);ptr(0x5c,0xb4);
        real(0xac,special?100:10);real(0xb8,special?100:0);real(0x68,special?2:1);real(0x6c,special?2000:1000);real(0x70,special?99:60);real(0x74,special?1.18f:4.f/3);
        ptr(0x90,0xc8);ptr(0xc8,0xe0);ptr(0xcc,0xf0);ptr(0xf0,0x100);real(0xe4,20);real(0x104,20);ptr(0xe8,0x110);ptr(0x108,0x130);
        word(0x114,18);block[0x11c]=10;ptr(0x120,0x160);word(0x134,18);block[0x13c]=5;ptr(0x140,0x180);stream(0x160,special?30:35,special?40:45);stream(0x180,special?5:3,special?15:13);
    }
    std::vector<gf::u8> archive()const{
        std::vector<gf::u8> b(32+block.size()+fields.size()*4+8+11);gf::store_be32(b.data(),b.size());gf::store_be32(b.data()+4,block.size());gf::store_be32(b.data()+8,fields.size());gf::store_be32(b.data()+12,1);std::copy(block.begin(),block.end(),b.begin()+32);unsigned at=32+block.size();for(auto field:fields){gf::store_be32(b.data()+at,field);at+=4;}at+=8;const char name[]="scene_data";std::copy(name,name+11,b.begin()+at);return b;
    }
};
} // namespace startup_camera_fixture
