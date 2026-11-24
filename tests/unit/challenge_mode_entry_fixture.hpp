#pragma once
#include <array>
#include <cstdlib>
#include "engine/assets/challenge_preparation_entry.hpp"
namespace challenge_mode_entry_fixture {
inline std::array<gf::u8,96> bytes(){
    using namespace gf;std::array<u8,96> result{};
    const std::array<u32,24> words{
        0x47465045,3,96,0x800bfdac,0x80195f74,0x80195fe0,6328,0x01000000,
        0,0,0,0x800284f4,0x80017918,0x80196948,0x01030100,0x802d2e48,
        0x8002821c,0x802cfdc4,0x80195f2c,1,0x801962c4,0x46,0,0x80195fbc};
    for(unsigned i=0;i<words.size();++i)store_be32(result.data()+4*i,words[i]);
    return result;
}
}
