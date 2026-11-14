#pragma once
#include <stdexcept>
#include <vector>
#include "engine/assets/story_force_cards.hpp"
namespace story_force_cards_fixture {
inline std::vector<gf::u8> cards(){using namespace gf;std::vector<u8> b(176);store_be32(b.data(),0x47464643);store_be32(b.data()+4,1);store_be32(b.data()+8,7);std::size_t at=12;
    for(unsigned i=0;i<4;++i){store_be16(b.data()+at,u16(i));store_be16(b.data()+at+2,i==3?0:4096);at+=4;}
    auto real=[&](f32 x){store_be_f32(b.data()+at,x);at+=4;};
    for(unsigned i=0;i<9;++i){real(-f32(i));real(2);}
    for(f32 x:{3.f,4.f,5.f,.1f,1.f,2.f,.5f,1.f,10.f,7.5f,.5f,35.f,30.f,12.f,8.f,1.f,25.5f,.25f,2.f})real(x);
    return b;
}
inline std::vector<gf::u8> presentation(){using namespace gf;std::vector<u8> b(84);store_be32(b.data(),0x4746534b);store_be32(b.data()+4,1);store_be32(b.data()+8,7);std::size_t at=12;
    for(f32 x:{0.f,0.f,0.f,0.f,0.f,-1.f,0.f,1.f,0.f,41.f,10.f,40000.f,4.f/3,0.f,0.f,-500.f,bits_to_f32(0x38c90fdb)}){store_be_f32(b.data()+at,x);at+=4;}
    store_be16(b.data()+80,4);return b;
}
inline gf::StoryForceCardArt art(){const auto a=cards(),b=presentation();gf::StoryForceCardArt result;std::string error;if(!result.load({a.data(),a.size()},{b.data(),b.size()},&error))throw std::runtime_error(error);return result;}
} //namespace story_force_cards_fixture
