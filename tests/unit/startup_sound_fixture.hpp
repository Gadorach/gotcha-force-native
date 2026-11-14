#pragma once
#include <vector>
#include "engine/core/types.hpp"
namespace startup_sound_fixture {
inline std::vector<gf::u8> profile(){
    using namespace gf;std::vector<u8> b(144);store_be32(b.data(),0x4746534d);store_be16(b.data()+4,1);store_be16(b.data()+6,5);store_be32(b.data()+8,144);store_be32(b.data()+16,0x3e4ccccd);
    const u16 ids[]={0,1,2,4,10},starts[]={40,126,200,204,204},stops[]={41,127,201,205,205},types[]={1,1,0,0,0};
    for(unsigned i=0;i<5;++i){auto* row=b.data()+24+i*24;store_be16(row,ids[i]);store_be16(row+4,starts[i]);store_be16(row+6,stops[i]);store_be16(row+8,types[i]);store_be32(row+12,10000);}
    return b;
}
}
