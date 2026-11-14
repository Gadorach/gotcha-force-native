#pragma once
#include <vector>
#include "engine/gameplay/sound_voice_envelope.hpp"
namespace sound_envelope_fixture {
inline std::vector<gf::u8> profile(){
    using namespace gf;std::vector<u8> bytes(528);store_be32(bytes.data(),0x47465645u);store_be16(bytes.data()+4,1);store_be16(bytes.data()+6,528);store_be32(bytes.data()+8,0x4d000000u);store_be32(bytes.data()+12,0x3c800000u);
    // Owned analytic table, NOT an original game table.
    for(unsigned i=0;i<128;++i)store_be32(bytes.data()+16+i*4,i*0x100000u);
    return bytes;
}
} // namespace sound_envelope_fixture
