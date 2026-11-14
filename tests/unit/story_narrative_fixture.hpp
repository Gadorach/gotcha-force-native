#pragma once
#include <vector>
#include "engine/gameplay/story_narrative_reference.hpp"
namespace story_narrative_fixture {
inline void fingerprint(std::vector<gf::u8>& b){gf::u32 hash=2166136261u;for(std::size_t i=24;i<b.size();++i)hash=(hash^b[i])*16777619u;gf::store_be32(b.data()+16,hash);}
inline std::vector<gf::u8> bank(){using namespace gf;
    const std::vector<std::vector<u16>> scripts{{0x011f,25,0x0218,0,7,0x011b,99,0x002e},{0x0014,0x002e},{0x0223,2,4,0x002e}};
    std::size_t size=0;for(const auto& s:scripts)size+=s.size()*2;std::vector<u8> b(24+scripts.size()*12+size);
    store_be32(b.data(),0x47465342);store_be16(b.data()+4,1);store_be16(b.data()+6,u16(scripts.size()));store_be32(b.data()+8,u32(size));store_be32(b.data()+12,123);
    std::size_t offset=0;for(unsigned i=0;i<scripts.size();++i){const std::size_t at=24+i*12;b[at]=u8(i==2);b[at+1]=u8(i==1);store_be16(b.data()+at+2,1);store_be32(b.data()+at+4,u32(offset));store_be32(b.data()+at+8,u32(scripts[i].size()*2));
        for(auto word:scripts[i]){store_be16(b.data()+60+offset,word);offset+=2;}}
    fingerprint(b);return b;
}
}
