#pragma once
#include "engine/gameplay/scene_descriptor_bootstrap.hpp"
namespace descriptor_bootstrap_fixture {
inline std::array<gf::u8,128> bytes(){using namespace gf;std::array<u8,128>b{};const std::array<u32,8> header{0x47464442,1,128,1,0x8036f644,0x80197854,0x8036f6b4,0};for(unsigned i=0;i<header.size();++i)store_be32(b.data()+4*i,header[i]);const std::array<SceneDescriptorBootstrapRow,4> rows{{{0x1de800,0x10000,44,0,1,0x80006f60,0x80006ed8},{0x1ee800,0x400,32,1,1,0x80006e3c,0x80006e8c},{0x1eec00,0x2000,16,2,1,0x80006e3c,0x80006e8c},{0x1f0c00,0x2000,16,3,1,0,0x80006f1c}}};for(unsigned i=0;i<4;++i){const auto& r=rows[i];const unsigned at=32+24*i;store_be32(b.data()+at,r.offset);store_be32(b.data()+at+4,r.bytes);store_be16(b.data()+at+8,r.stride);b[at+10]=r.pool;b[at+11]=1;store_be32(b.data()+at+12,r.initialize_key);store_be32(b.data()+at+16,r.cleanup_key);}return b;}
}
