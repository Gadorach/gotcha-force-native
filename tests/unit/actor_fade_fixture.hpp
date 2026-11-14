#pragma once
#include <vector>
#include "engine/gameplay/actor_fade.hpp"
namespace actor_fade_fixture {
inline std::vector<gf::u8> profile(){
    std::vector<gf::u8> b(80);gf::store_be32(b.data(),0x47464146);gf::store_be16(b.data()+4,1);gf::store_be16(b.data()+6,2);gf::store_be32(b.data()+8,80);
    for(unsigned i=0;i<2;++i){const unsigned at=24+i*28;b[at]=i?14:12;gf::store_be32(b.data()+at+8,i?0xff8040ffu:0x0a50c8ffu);gf::store_be_f32(b.data()+at+16,4);gf::store_be32(b.data()+at+20,i?0x000000ffu:0x6e2864ffu);gf::store_be32(b.data()+at+24,i?0xf0000000u:0xff000001u);}
    return b;
}
inline std::array<gf::u32,11> fields(const gf::ActorFadeState& s){
    std::array<gf::u32,11> values{};unsigned i=0;for(const auto& c:s.channels){values[i++]=c.curve;values[i++]=c.loops;values[i++]=gf::f32_to_bits(c.time);values[i++]=c.rgba;values[i++]=c.active;}values[i]=s.material_rgba;return values;
}
} // namespace actor_fade_fixture
