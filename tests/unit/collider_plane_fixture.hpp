#pragma once
#include <stdexcept>
#include "engine/gameplay/collider_plane_shape.hpp"
namespace collider_plane_fixture {
inline void put(std::vector<gf::u8>& b,unsigned at,gf::u32 value){for(unsigned i=0;i<4;++i)b[at+i]=gf::u8(value>>(24-8*i));}
inline gf::ColliderGeometryProfile profile(){return {{0,0x3f800000,0x42480000,0x3c8efa35,0x42652ee1,0x3fc90fdb,0x3f060a92,0x3c23d70a},true};}
inline std::vector<gf::u8> bank(){std::vector<gf::u8> b(7840);std::fill_n(b.begin(),32,0xff);std::fill_n(b.begin()+0xa20,128,0xff);b[0]=b[1]=0;b[2]=0x80;b[3]=31;const unsigned at=32;b[at]=4;b[at+1]=0x80;b[at+3]=0x20;put(b,at+12,0);put(b,at+16,gf::f32_to_bits(5));put(b,at+20,0);put(b,at+24,gf::f32_to_bits(1));put(b,at+28,gf::f32_to_bits(10));b[at+8]=b[at+9]=0xff;b[0xa20]=0;b[0xa21]=0xfe;b[0xa22]=63;return b;}
inline gf::CharacterCollisionDescriptor descriptor(){auto b=bank();gf::CharacterCollisionBank bank;gf::CharacterCollisionDescriptor d;std::string error;if(!bank.load({b.data(),b.size()},&error)||!bank.descriptor(gf::CharacterCollisionTable::Body,0,&d,&error))throw std::runtime_error(error);return d;}
}
