#pragma once
// Entirely owned published rigid three-joint record with joint/material AObjs.
#include <algorithm>
#include <vector>
#include "engine/core/types.hpp"
namespace hsd_record_animation_fixture {
struct Fixture {
    std::vector<gf::u8> block=std::vector<gf::u8>(0x1000);
    std::vector<gf::u32> fields;
    void word(unsigned at,gf::u32 value){gf::store_be32(block.data()+at,value);}
    void real(unsigned at,float value){word(at,gf::f32_to_bits(value));}
    void ptr(unsigned at,unsigned target){word(at,target);if(std::find(fields.begin(),fields.end(),at)==fields.end())fields.push_back(at);}
    void stream(unsigned at,float a,float b,unsigned wait=4,unsigned op=2){
        for(unsigned i=0;i<3;++i){block[at+i*6]=gf::u8(op);const auto bits=gf::f32_to_bits(i?b:a);
            for(unsigned k=0;k<4;++k)block[at+i*6+1+k]=gf::u8(bits>>(8*k));
            block[at+i*6+5]=gf::u8(wait);}
    }
    void aobj(unsigned at,unsigned track,unsigned data,unsigned channel,float a,float b){
        real(at+4,4);ptr(at+8,track);word(track+4,18);block[track+12]=gf::u8(channel);ptr(track+16,data);stream(data,a,b);
    }
    Fixture(){
        ptr(0x10,0x20);ptr(0x20,0x30);ptr(0x30,0x100);ptr(0x34,0x50);ptr(0x38,0x60);ptr(0x50,0x600);ptr(0x60,0x800);
        for(unsigned i=0;i<3;++i){const unsigned j=0x100+i*64,d=0x200+i*16,m=0x240+i*32,mat=0x300+i*32;
            word(j+4,0x80);if(i<2)ptr(j+8,j+64);ptr(j+16,d);
            for(unsigned k=0;k<3;++k)real(j+32+k*4,1);
            real(j+48,float(i));ptr(d+8,m);ptr(d+12,0x380);word(m+4,0x201d);ptr(m+12,mat);
            word(mat,0xffffffffu);word(mat+4,0xffffffffu);word(mat+8,0xffffffffu);real(mat+12,.75f);
        }
        ptr(0x388,0x400);word(0x38c,0x80000003);ptr(0x390,0x480);
        for(unsigned i=0;i<2;++i){unsigned at=0x400+i*24;word(at,i?10:9);word(at+4,1);word(at+8,i?0:1);word(at+12,4);}
        word(0x430,255);block[0x480]=0x90;gf::store_be16(block.data()+0x481,3);
        const float points[3][3]={{-1,-1,0},{0,1,0},{1,-1,0}};
        for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k){real(0x483+i*24+k*4,points[i][k]);real(0x48f+i*24+k*4,k==2?1.f:0.f);}
        ptr(0x600,0x620);ptr(0x620,0x640);ptr(0x608,0x700);ptr(0x628,0xb00);
        aobj(0x700,0x740,0x780,5,0,4);ptr(0x740,0x760);word(0x764,18);block[0x76c]=12;ptr(0x770,0x7a0);stream(0x7a0,1,1);
        aobj(0xb00,0xb40,0xbc0,11,1,1);
        for(unsigned i=0;i<3;++i){const auto j=0x800+i*32,mat=0x900+i*32,a=0x980+i*32,t=0xa00+i*32,data=0xc00+i*32;
            if(i<2)ptr(j,j+32);
            ptr(j+8,mat);ptr(mat+4,a);aobj(a,t,data,i==0?4:(i==1?10:7),i==0?1.5f:(i==1?-.5f:.5f),i==0?1.5f:(i==1?-.5f:.5f));
        }
    }
    std::vector<gf::u8> archive(int omit=-1)const{
        auto reloc=fields;reloc.erase(std::remove(reloc.begin(),reloc.end(),gf::u32(omit)),reloc.end());
        const char name[]="scene_data";const auto symbols=32+block.size()+reloc.size()*4;
        std::vector<gf::u8> bytes(symbols+8+sizeof(name));
        gf::store_be32(bytes.data(),gf::u32(bytes.size()));gf::store_be32(bytes.data()+4,gf::u32(block.size()));gf::store_be32(bytes.data()+8,gf::u32(reloc.size()));gf::store_be32(bytes.data()+12,1);
        std::copy(block.begin(),block.end(),bytes.begin()+32);
        for(unsigned i=0;i<reloc.size();++i)gf::store_be32(bytes.data()+32+block.size()+i*4,reloc[i]);
        gf::store_be32(bytes.data()+symbols,0x10);std::copy_n(name,sizeof(name),bytes.data()+symbols+8);return bytes;
    }
};
} // namespace hsd_record_animation_fixture
