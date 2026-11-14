#pragma once
#include <vector>
#include "engine/gameplay/startup_actor_setup.hpp"
namespace startup_actor_setup_fixture {
inline gf::u32 hash(gf::ByteView bytes){gf::u32 h=2166136261u;for(std::size_t i=0;i<bytes.size;++i){h^=bytes.data[i];h*=16777619u;}return h;}
inline std::vector<gf::u8> profile(){
    using namespace gf;std::vector<u8> b(1064);store_be32(b.data(),0x47464353);store_be16(b.data()+4,1);store_be16(b.data()+6,0x615);store_be32(b.data()+8,1234);store_be32(b.data()+12,b.size());
    store_be16(b.data()+16,432);store_be16(b.data()+18,192);store_be16(b.data()+20,32);for(unsigned i=0;i<4;++i)store_be32(b.data()+24+i*4,std::array<u32,4>{48,480,672,1056}[i]);
    store_be16(b.data()+48+4,1234);b[48+0xa1]=31;b[48+0xa2]=8;
    for(unsigned i=0;i<32;++i)for(unsigned k=0;k<3;++k)store_be_f32(b.data()+672+i*12+k*4,k?1.f:1.f+f32(i)*.1f);
    for(auto [at,v]:std::array<std::pair<unsigned,float>,5>{{{0x44,30},{0x48,5},{0x4c,80},{0x50,45},{0x54,20}}})store_be_f32(b.data()+480+at,v);
    for(unsigned k=0;k<3;++k)store_be_f32(b.data()+48+0x74+k*4,f32(k)-1);
    store_be_f32(b.data()+1056,1);store_be_f32(b.data()+1060,80);return b;
}
inline std::array<gf::u32,5> rank_fields(const gf::StartupActorRankState& s){return {s.requested,s.selected,gf::f32_to_bits(s.scale_b8),gf::f32_to_bits(s.field1dd0),gf::f32_to_bits(s.field5f8)};}
inline std::array<gf::u32,25> fields(const gf::StartupActorInitialSetup& s){
    return {s.rank.requested,s.rank.selected,gf::f32_to_bits(s.rank.scale_b8),gf::f32_to_bits(s.rank.field1dd0),gf::f32_to_bits(s.rank.field5f8),
            gf::f32_to_bits(s.field5f4),gf::f32_to_bits(s.field768),gf::f32_to_bits(s.scale_b4),gf::f32_to_bits(s.fieldc0),gf::f32_to_bits(s.fieldc4),
            gf::f32_to_bits(s.actor_scale[0]),gf::f32_to_bits(s.actor_scale[1]),gf::f32_to_bits(s.actor_scale[2]),gf::u32(s.health736),gf::u32(s.health738),
            gf::f32_to_bits(s.field668),gf::f32_to_bits(s.field66c),gf::f32_to_bits(s.field670),gf::f32_to_bits(s.field674),gf::f32_to_bits(s.field770),gf::f32_to_bits(s.field7fc),
            gf::f32_to_bits(s.fields678[0]),gf::f32_to_bits(s.fields678[1]),gf::f32_to_bits(s.fields678[2]),gf::u32(s.numeric_setup_only)};
}
} // namespace startup_actor_setup_fixture
