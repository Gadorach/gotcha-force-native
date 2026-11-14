#pragma once
#include "engine/assets/battle_character_resources.hpp"
namespace battle_character_resource_fixture {
inline void put(std::vector<gf::u8>& b,unsigned at,gf::u32 value){for(unsigned i=0;i<4;++i)b[at+i]=gf::u8(value>>(24-8*i));}
inline std::vector<gf::u8> bundle(){
    using namespace gf;std::array<std::vector<u8>,10> slots;slots[0].resize(432);put(slots[0],0,0x12345678);slots[2].resize(8,0xa5);slots[3].resize(64,0x3c);
    for(unsigned variant=0;variant<6;++variant){slots[4+variant].resize(52);put(slots[4+variant],0,16);put(slots[4+variant],16,32);put(slots[4+variant],20,variant+1);slots[4+variant][51]=0xb7;}
    std::size_t total=96;for(const auto& s:slots)total+=s.size();std::vector<u8> b(total);put(b,0,0x47464e42);put(b,4,1);put(b,8,10);put(b,12,u32(total-16));unsigned at=96;for(unsigned i=0;i<10;++i){put(b,16+8*i,i);put(b,20+8*i,u32(slots[i].size()));std::copy(slots[i].begin(),slots[i].end(),b.begin()+at);at+=unsigned(slots[i].size());}return b;
}
inline gf::BattleCharacterResourceRow row(const std::vector<gf::u8>& b){return {8,0,9000,gf::u32(b.size()),gf::challenge_payload_hash({b.data(),b.size()}),10,432,6};}
inline std::vector<gf::u8> profile(const gf::BattleCharacterResourceRow& r){using namespace gf;std::vector<u8> b(56);put(b,0,0x4746434b);put(b,4,1);b[9]=1;b[11]=24;put(b,12,24);b[32]=u8(r.model>>8);b[33]=u8(r.model);b[34]=u8(r.number>>8);b[35]=u8(r.number);put(b,36,r.member);put(b,40,r.bytes);put(b,44,r.hash);b[48]=u8(r.slots>>8);b[49]=u8(r.slots);b[50]=r.variants;b[52]=u8(r.actor_bytes>>8);b[53]=u8(r.actor_bytes);return b;}
} // namespace battle_character_resource_fixture
