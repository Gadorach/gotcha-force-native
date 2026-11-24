#pragma once
#include <array>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct BattleActorProgramSeedWrite {u16 offset=0;u8 bytes=0;u16 value=0;};
struct BattleActorProgramSeed {
    std::array<u32,2> fields1e0{};std::array<u8,13> flags{};u16 field26c=0;
    std::array<u16,48> fields20c{};u16 flags272=0xef;
    std::vector<BattleActorProgramSeedWrite> writes;
};
//3CC24's reset fields only. Binding3FC belongs to the native resource owner;
//these clears do not execute a battle program or create collision geometry.
inline BattleActorProgramSeed battle_actor_program_seed(){
    BattleActorProgramSeed p;
    p.writes.push_back({0x1e0,4,0});p.writes.push_back({0x1e4,4,0});
    for(u16 at:{u16(0x1d9),u16(0x1da),u16(0x1db),u16(0x1de),u16(0x1dc),u16(0x1dd),u16(0x280),u16(0x703),u16(0x704),u16(0x26e)})p.writes.push_back({at,1,0});
    p.writes.push_back({0x26c,2,0});
    for(u16 at:{u16(0x274),u16(0x1eb),u16(0x276)})p.writes.push_back({at,1,0});
    for(unsigned i=0;i<48;++i)p.writes.push_back({u16(0x20c+2*i),2,0});
    p.writes.push_back({0x272,2,0xef});return p;
}
} // namespace gf
