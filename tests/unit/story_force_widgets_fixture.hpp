#pragma once
#include "tests/unit/story_force_cards_fixture.hpp"
#include "engine/assets/story_force_widgets.hpp"
namespace story_force_widgets_fixture {
inline std::vector<gf::u8> bytes(){using namespace gf;std::vector<u8> b(280);store_be32(b.data(),0x47465357);store_be32(b.data()+4,1);store_be32(b.data()+8,7);std::size_t at=12;
    for(unsigned group=0;group<3;++group){for(unsigned i=0;i<4;++i){store_be16(b.data()+at,u16(50+group*4+i));at+=2;}store_be_f32(b.data()+at,f32(group)*20-20);at+=4;}
    for(unsigned i=0;i<6;++i){store_be16(b.data()+at,u16(i<4?i==0?9:i==1?38:39:i==4?7:8));at+=4;for(unsigned k=0;k<3;++k){store_be_f32(b.data()+at,k==0?f32(i):0);at+=4;}}
    const f32 values[30]={3,120,0,60,-270,9,.15f,-1,-.15f,-96,-297,7.4f,.5f,.4f,-71,-114,-480,10,8,.5f,28,12.75f,-137,-109,-479,17.36f,0,-14,-330,bits_to_f32(0x38c90fdb)};
    for(f32 f:values){store_be_f32(b.data()+at,f);at+=4;}store_be32(b.data()+at,0xe0e0e000);store_be32(b.data()+at+4,0x0050e000);store_be16(b.data()+at+8,2548);store_be16(b.data()+at+10,98);store_be16(b.data()+at+12,24);store_be16(b.data()+at+14,6);return b;
}
inline gf::StoryForceWidgetsArt art(){const auto b=bytes();gf::StoryForceWidgetsArt result;std::string error;if(!result.load({b.data(),b.size()},story_force_cards_fixture::art(),&error))throw std::runtime_error(error);return result;}
}
