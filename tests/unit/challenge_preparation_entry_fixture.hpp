#pragma once
#include "engine/assets/challenge_preparation_entry.hpp"
#include <array>
namespace preparation_entry_fixture {
inline std::array<gf::u8,64> bytes(){std::array<gf::u8,64> b{};const std::array<gf::u32,16> words{0x47465045,1,64,0x800bfdac,0x80195f74,0x80195fe0,6328,0x01000000,0,0,0,0x800284f4,0,0,0,0};for(unsigned i=0;i<words.size();++i)gf::store_be32(b.data()+4*i,words[i]);return b;}
}
