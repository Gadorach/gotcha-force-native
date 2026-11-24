#pragma once
#include "engine/gameplay/battle_initial_briefing_type2.hpp"
namespace briefing_type2_fixture {
inline std::array<gf::u8,352> bytes(){
    using namespace gf;std::array<u8,352> b{};
    const std::array<u32,32> h{0x47464232,1,352,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,0x8019a3f0,0x8019bf18,0x80006fb4,0x80005630,0x800086b8,0x80007c30,0x80007cd0,0x80089100,0x22e678b4,27,2,18,57,0x80047aa4,0x20,4,0x80370ff8,0,14,15,22,23,0xc3480000,0xc28c0000};
    const std::array<u32,54> t{
        0xc2b8cfdf,0xc120bd3c,0xc3968000,0xc2758106,0xc2ac22f8,0xc3968000,0xc2b8cfdf,0xc2ac22f8,0xc3968000,
        0x42758106,0x4239020c,0xc3968000,0x42b8cfdf,0xc1ed999a,0xc3968000,0x42758106,0xc1ed999a,0xc3968000,
        0xc2fefb64,0x4092432d,0xc395e000,0xc2c0ec4a,0xc28ee711,0xc395e000,0xc2fefb64,0xc28ee711,0xc395e000,
        0x42c0ec4a,0x4273020c,0xc395e000,0x42fefb64,0xc1745254,0xc395e000,0x42c0ec4a,0xc1745254,0xc395e000,
        0xc2ddda93,0x3ff542c4,0xc39a0000,0xc29fcb78,0xc2943639,0xc39a0000,0xc2ddda93,0xc2943639,0xc39a0000,
        0x429fcb78,0x426863bd,0xc39a0000,0x42ddda93,0xc18f65c9,0xc39a0000,0x429fcb78,0xc18f65c9,0xc39a0000};
    for(unsigned i=0;i<h.size();++i)store_be32(b.data()+4*i,h[i]);
    for(unsigned i=0;i<t.size();++i)store_be32(b.data()+128+4*i,t[i]);
    return b;
}
inline gf::BattleInitialBriefingType2Facts facts(){
    gf::BattleInitialBriefingType2Facts f;f.active=15;f.humans=10;f.teams={1,0,1,0,-1,-1};f.counts={2,2};f.slots={{{1,3,-1,-1,-1,-1},{0,2,-1,-1,-1,-1}}};return f;
}
}
