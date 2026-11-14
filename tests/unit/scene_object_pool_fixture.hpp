#pragma once
#include <cstdlib>
#include "engine/gameplay/scene_object_pool.hpp"
namespace scene_object_fixture {
inline gf::SceneDescriptorPoolProfile descriptors(unsigned n){using namespace gf;std::array<u8,40>b{};store_be32(b.data(),0x47464450);store_be32(b.data()+4,1);store_be32(b.data()+8,40);store_be32(b.data()+16,1);store_be32(b.data()+20,44*n);store_be16(b.data()+24,44);b[27]=1;store_be32(b.data()+28,0x80006f60);store_be32(b.data()+32,0x80006ed8);SceneDescriptorPoolProfile p;std::string e;if(!p.load({b.data(),40},&e))std::abort();return p;}
inline std::array<gf::u8,96> bytes(unsigned n){using namespace gf;std::array<u8,96>b{};store_be32(b.data(),0x47464f50);store_be32(b.data()+4,1);store_be32(b.data()+8,96);store_be16(b.data()+16,u16(n));store_be16(b.data()+18,452);b[20]=b[21]=2;for(unsigned i=0;i<64;++i)b[32+i]=u8(i%7);return b;}
inline gf::SceneObjectPoolProfile objects(unsigned n){auto b=bytes(n);gf::SceneObjectPoolProfile p;std::string e;if(!p.load({b.data(),b.size()},&e))std::abort();return p;}
inline std::shared_ptr<gf::SceneDescriptorPool> owner(unsigned n){auto p=std::make_shared<gf::SceneDescriptorPool>();std::string e;if(!p->begin_fresh(descriptors(n),&e))std::abort();return p;}
}
