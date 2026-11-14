#pragma once
#include <bit>
#include "engine/assets/startup_music.hpp"
namespace startup_music_fixture {
inline void word(std::vector<gf::u8>& out,unsigned offset,gf::u32 value){gf::store_be32(out.data()+offset,value);}
inline std::vector<gf::u8> menu(){
    std::vector<gf::u8> out(80);const gf::u32 words[]={0x47464d42,1,123,2,0,1000,456,1,256,64,789,44100,32,0,32,1,48000,0,2,0};
    for(unsigned i=0;i<20;++i)word(out,i*4,words[i]);
    return out;
}
inline std::vector<gf::u8> profile(){
    const auto parent=menu();std::vector<gf::u8> out(200);word(out,0,0x47464d53);word(out,4,1);word(out,8,123);
    word(out,12,gf::music_hash({parent.data(),parent.size()}));word(out,16,27);word(out,20,29);word(out,28,2);word(out,192,1);
    std::copy(parent.begin(),parent.end(),out.begin()+32);std::copy(parent.begin(),parent.end(),out.begin()+112);
    word(out,32+28,0);word(out,32+32,128);word(out,32+40,790);return out;
}
}
