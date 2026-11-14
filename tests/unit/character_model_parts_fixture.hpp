#pragma once
#include "engine/assets/character_model_parts.hpp"
#include <vector>
namespace character_model_parts_fixture {
inline std::vector<gf::u8> profile(gf::u32 hash,bool red=true){using namespace gf;const unsigned n=red?8:10;std::vector<u8> b(24+n*4);store_be32(b.data(),0x47465054);store_be16(b.data()+4,1);store_be16(b.data()+6,red?0x615:10);store_be32(b.data()+8,hash);store_be32(b.data()+12,b.size());store_be16(b.data()+16,n);b[18]=2;
    for(unsigned i=0;i<n;++i){auto* row=b.data()+24+i*4;row[0]=u8(i+1);row[1]=i<5?7:11;row[2]=i==0?0:i==5?1:255;}return b;}
}
