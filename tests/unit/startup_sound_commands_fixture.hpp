#pragma once
#include "engine/gameplay/startup_sound_commands.hpp"
namespace startup_sound_commands_fixture {
inline std::vector<gf::u8> tones(){
    using namespace gf;std::vector<u8> b(2064);b[0]='T';b[1]='S';b[2]='B';b[3]='D';b[4]=0x10;b[5]=8;b[9]=1;b[12]=128;
    for(unsigned i=0;i<128;++i){b[16+16*i]=255;b[16+16*i+15]=255;}
    const std::array<u8,16> start={0,0,1,27,0,0,0x85,100,90,255,0,0,0,0,0,255};
    auto stop=start;stop[0]=1;stop[8]=0;
    for(unsigned i=0;i<16;++i){b[16+40*16+i]=start[i];b[16+41*16+i]=stop[i];}
    return b;
}
inline std::vector<gf::u8> profile(const std::vector<gf::u8>& tones){
    using namespace gf;std::vector<u8> b(48);store_be32(b.data(),0x47465343);store_be16(b.data()+4,1);store_be16(b.data()+6,5);store_be32(b.data()+8,48);store_be32(b.data()+16,startup_sound_commands_asset_detail::fnv({tones.data(),tones.size()}));
    const u32 values[]={0x3f19999a,0x3f333333,0x3f4ccccd,0x3f666666,0x3f800000};for(unsigned i=0;i<5;++i)store_be32(b.data()+20+4*i,values[i]);return b;
}
}
