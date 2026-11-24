#pragma once
#include "engine/gameplay/battle_initial_briefing_type5.hpp"
#include "engine/gameplay/battle_initial_briefing_type6.hpp"
namespace briefing_cards_fixture {
inline std::array<gf::u8,208> five(){
    using namespace gf;std::array<u8,208> b{};
    const std::array<u32,32> h{0x47464235,1,208,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,0x8019b358,0x8019bf18,0x80006fb4,0x80007d68,0x800092bc,0x80089100,0x80047aa4,0x80350d0c,0x87d2f859,202,206,0x42d858f4,5,36,5,1,0x11,15,0x41a60000,0x42260000,0xc197999a,0x3ecccccd,0xc3480000,0x80371130};
    const std::array<u32,18> t{0xc2bf0000,0xc03b573f,0xc3970000,0xc280f0a4,0xc29de60b,0xc3970000,0xc280f0a4,0xc29de60b,0xc3970000,0x4280f0a4,0x42557be7,0xc3970000,0x42bf0000,0xc1b4a5e3,0xc3970000,0x42bf0000,0xc1b4a5e3,0xc3970000};
    for(unsigned i=0;i<h.size();++i)store_be32(b.data()+4*i,h[i]);
    for(unsigned i=0;i<t.size();++i)store_be32(b.data()+128+4*i,t[i]);
    return b;
}
inline std::array<gf::u8,128> six(){
    using namespace gf;std::array<u8,128> b{};const std::array<u32,32> w{0x47464236,1,128,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,0x8019b8a4,0x80006fb4,0x80089100,0x80047aa4,0x80199d30,0x8019bb34,0x80005630,0x80370fb8,0x22e678b4,27,6,0,1,21,0x20,4,0x19,0x12,8,0x444,0x3f4ccccd,0xc3c80000,0x3f800000,0x38};
    for(unsigned i=0;i<w.size();++i)store_be32(b.data()+4*i,w[i]);
    return b;
}
}
