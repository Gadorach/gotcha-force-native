#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <bit>
namespace gf {
//Original801D79D0 row +801D7AA0/7B64 operands, not a caller-ready flag.
struct StoryGuideIndicatorArt {
    u32 bank_hash=0;u16 record=0;std::array<f32,3> position{};f32 scale=0,offset=0,factor=0,width_origin=0;bool ready=false;
    bool valid()const{return ready&&bank_hash&&record==2&&position==std::array<f32,3>{0,-52,-399.5f}&&scale==1&&f32_to_bits(offset)==0xc1e6d07du&&f32_to_bits(factor)==0x40847d56u&&width_origin==5;}
    bool load(ByteView b){
        *this={};if(!b.data||b.size!=48||b.be32(0)!=0x47464749||b.be32(4)!=1||!b.be32(8)||b.be32(12)!=2||b.be32(16))return false;
        bank_hash=b.be32(8);record=2;for(unsigned i=0;i<3;++i)position[i]=b.bef32(20+i*4);scale=b.bef32(32);offset=b.bef32(36);factor=b.bef32(40);width_origin=b.bef32(44);
        if(position!=std::array<f32,3>{0,-52,-399.5f}||scale!=1||f32_to_bits(offset)!=0xc1e6d07du||f32_to_bits(factor)!=0x40847d56u||width_origin!=5)return false;
        ready=true;return true;
    }
};
} //namespace gf
