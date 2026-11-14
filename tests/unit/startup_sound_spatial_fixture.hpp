#pragma once
#include <vector>
#include "engine/gameplay/startup_sound_spatial.hpp"
namespace startup_sound_spatial_fixture {
inline std::vector<gf::u8> profile(){
    using namespace gf;std::vector<u8> b(64);store_be32(b.data(),0x47465350);store_be16(b.data()+4,1);store_be16(b.data()+6,11);store_be32(b.data()+8,64);
    const u32 words[]={0x3e4ccccd,0xbf800000,0,0x42be0000,0x42200000,0x44200000,0x40a14285,0x3c23d70a,0,0x3f800000,0x3f000000};
    for(unsigned i=0;i<11;++i)store_be32(b.data()+16+4*i,words[i]);
    return b;
}
inline gf::StartupSoundSpatialSnapshot snapshot(){
    gf::StartupSoundSpatialSnapshot s;s.epoch=77;s.scene_declared=s.bias_selection_declared=s.projection_declared=true;
    s.viewers0c={0,-1,-1,-1};s.projection={0,1,0,1,0,-1,-1};s.viewport={0,0,640,480,0,1};
    for(auto& v:s.viewers){v.epoch=77;v.actor_declared=v.position_declared=v.facing_declared=v.camera_declared=true;v.actor2e5=0;v.position2e8={0,0,10};v.matrix364.at(2,3)=10;v.matrix394.at(2,3)=-30;}
    return s;
}
inline gf::StartupSoundOwnerObservation owner(){return {101,77,true,true,0,{0,0,20}};}
}
