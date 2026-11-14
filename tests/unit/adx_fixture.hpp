#pragma once
#include "engine/assets/adx.hpp"
namespace owned_adx {
inline std::vector<gf::u8> file(gf::u32 frames=4,unsigned channels=2,bool loop=false){
    std::vector<gf::u8> b(64+((frames+31)/32)*channels*18);
    auto w=[&](unsigned at,gf::u32 v){gf::store_be32(b.data()+at,v);};
    w(0,0x8000003c);w(4,0x03120400u|channels);w(8,8000);w(12,frames);w(16,0x00000300);
    w(24,loop?1:0);if(loop){w(28,0);w(32,64);w(36,frames);w(40,gf::u32(b.size()));}
    const char signature[]="(c)CRI";std::copy(signature,signature+6,b.begin()+58);
    for(std::size_t at=64;at<b.size();at+=18){b[at+1]=1;b[at+2]=0x12;b[at+3]=0xfe;}
    return b;
}
}
