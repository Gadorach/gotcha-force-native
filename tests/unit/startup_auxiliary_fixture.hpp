#pragma once
#include "engine/gameplay/startup_auxiliary.hpp"
#include "tests/unit/character_model_fixture.hpp"
namespace startup_auxiliary_fixture {
inline std::vector<gf::u8> model(){
    auto b=character_model_fixture::archive();using namespace character_model_fixture;
    word(b,32+0x44,0x20080080);word(b,32+0x84,0x20080080);scalar(b,32+0x6c,999);
    word(b,32+0x15c,0x80000003);word(b,32+0x164,0);word(b,32+0x138,0);word(b,32+0x134,0x60006001);scalar(b,32+0x28c,.99f);return b;
}
inline gf::u32 hash(const std::vector<gf::u8>& b){gf::u32 h=2166136261u;for(auto v:b){h^=v;h*=16777619u;}return h;}
inline std::vector<gf::u8> profile(const std::vector<gf::u8>& b){
    using namespace gf;std::vector<u8> p(160);store_be32(p.data(),0x47464155);store_be16(p.data()+4,1);store_be16(p.data()+6,2);store_be32(p.data()+8,160);store_be32(p.data()+12,hash(b));store_be32(p.data()+16,0x40);store_be16(p.data()+20,2);store_be16(p.data()+22,2);
    for(unsigned i=0;i<2;++i){const auto at=24+i*52;p[at]=u8(40+i);p[at+1]=u8(i*2);p[at+2]=2;
        for(unsigned n=0;n<2;++n){const auto q=at+4+n*24;p[q]=51;p[q+1]=3;store_be16(p.data()+q+2,i?0:64);store_be16(p.data()+q+4,16384);store_be_f32(p.data()+q+8,n?-2.f:2.f);store_be_f32(p.data()+q+12,3);store_be_f32(p.data()+q+16,4);store_be_f32(p.data()+q+20,1);}
        for(unsigned k=0;k<3;++k)store_be_f32(p.data()+128+i*12+k*4,float(i?3-k:k+1));
    }
    store_be32(p.data()+152,0x38c90fdb);store_be16(p.data()+156,0x615);store_be16(p.data()+158,51);return p;
}
} // namespace startup_auxiliary_fixture
