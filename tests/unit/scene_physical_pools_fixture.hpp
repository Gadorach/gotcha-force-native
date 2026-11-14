#pragma once
#include "engine/gameplay/scene_physical_pools.hpp"
#include "tests/unit/scene_descriptor_bootstrap_fixture.hpp"
#include <stdexcept>
namespace physical_pools_fixture {
inline std::array<gf::u8,224> bytes(){using namespace gf;std::array<u8,224>b{};const std::array<u32,8> h{0x4746504f,1,224,0,0xfe800,0xe0000,0xca500,0};for(unsigned i=0;i<8;++i)store_be32(b.data()+4*i,h[i]);const std::array<ScenePhysicalPoolRow,8> r{{{0,6,7680,0,0},{0xb400,1,256,1,1},{0xb500,512,452,2,2},{0x43d00,512,740,3,3},{0xa0500,384,448,4,4},{0xca500,0,0,5,5},{0xca500,0,0,6,6},{0xca500,0,0,7,7}}};for(unsigned i=0;i<8;++i){const unsigned a=32+16*i;store_be32(b.data()+a,r[i].relative_offset);store_be16(b.data()+a+4,r[i].count);store_be16(b.data()+a+6,r[i].stride);b[a+8]=r[i].physical;b[a+9]=r[i].source_class;}for(unsigned i=0;i<64;++i)b[160+i]=gf::u8((7*i+3)&31);return b;}
inline gf::SceneDescriptorBootstrapProfile descriptor_profile(){gf::SceneDescriptorBootstrapProfile p;std::string e;const auto b=descriptor_bootstrap_fixture::bytes();if(!p.load({b.data(),b.size()},&e))throw std::runtime_error(e);return p;}
}
