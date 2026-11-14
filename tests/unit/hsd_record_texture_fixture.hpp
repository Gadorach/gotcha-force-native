#pragma once
#include <array>
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace hsd_record_texture_fixture {
inline auto fixture(unsigned channel=2,float first=-.25f,float last=-.5f,bool colours=false){
    hsd_record_animation_fixture::Fixture f;
    f.ptr(0x248,0xe00);f.word(0xe0c,4);
    for(unsigned k=0;k<3;++k)f.real(0xe1c+k*4,1);
    f.block[0xe3c]=f.block[0xe3d]=1;f.word(0xe40,0x00040010);f.real(0xe44,1);f.ptr(0xe4c,0xe80);
    f.ptr(0xe80,0xec0);gf::store_be16(f.block.data()+0xe84,8);gf::store_be16(f.block.data()+0xe86,8);f.word(0xe88,0);
    f.ptr(0x908,0xd00);f.ptr(0xd08,0xd20);f.aobj(0xd20,0xd40,0xd60,channel,first,last);
    if(colours){
        f.ptr(0xe58,0xf00);const std::array<gf::u8,16> expression={0,0,0,0,0,0,1,1,0x85,0x80,8,15,7,7,7,7};std::copy(expression.begin(),expression.end(),f.block.begin()+0xf00);f.word(0xf10,0xffffffffu);f.word(0xf14,0);f.word(0xf1c,0x40000000u);
    }
    return f;
}
}
