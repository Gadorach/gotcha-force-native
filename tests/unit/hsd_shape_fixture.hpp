#pragma once
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace hsd_shape_fixture {
inline auto fixture(){
    hsd_record_animation_fixture::Fixture f;
    for(unsigned at:{0x34u,0x38u,0x108u}){f.word(at,0);f.fields.erase(std::remove(f.fields.begin(),f.fields.end(),at),f.fields.end());}
    f.word(0x38c,0x90000003);f.ptr(0x394,0xd60);f.word(0x404,2);gf::store_be16(f.block.data()+0x412,12);f.ptr(0x414,0xd00);f.word(0x41c,0);
    std::fill(f.block.begin()+0x480,f.block.begin()+0x4e0,0);f.block[0x480]=0x90;gf::store_be16(f.block.data()+0x481,3);f.block[0x483]=0;f.block[0x484]=1;f.block[0x485]=2;
    const float points[3][3]={{-1,-1,0},{0,1,0},{1,-1,0}};
    for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k){f.real(0xd00+i*12+k*4,points[i][k]);f.real(0xe40+i*12+k*4,points[i][k]);f.real(0xe40+(i+3)*12+k*4,points[i][k]*2);}
    gf::store_be16(f.block.data()+0xd60,5);gf::store_be16(f.block.data()+0xd62,2);f.word(0xd64,3);f.ptr(0xd68,0xda0);f.ptr(0xd6c,0xdc0);
    f.word(0xda0,9);f.word(0xda4,2);f.word(0xda8,1);f.word(0xdac,4);gf::store_be16(f.block.data()+0xdb2,12);f.ptr(0xdb4,0xe40);f.ptr(0xdc0,0xde0);f.ptr(0xdc4,0xde8);
    for(unsigned i=0;i<3;++i){f.block[0xde0+i]=gf::u8(i);f.block[0xde8+i]=gf::u8(i+3);}
    f.ptr(0x3c,0xf80);f.ptr(0xf80,0xec0);f.ptr(0xec8,0xed0);f.ptr(0xed4,0xee0);f.ptr(0xee4,0xef0);f.aobj(0xef0,0xf10,0xf40,1,0,1);
    return f;
}
}
