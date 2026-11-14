#pragma once
#include "tests/unit/battle_preparation_models_fixture.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
namespace scene_archive_fixture {
inline std::array<gf::u8,40> bytes(unsigned count=4){using namespace gf;std::array<u8,40>b{};store_be32(b.data(),0x47464150);store_be32(b.data()+4,1);store_be32(b.data()+8,40);store_be32(b.data()+16,0x1ee800);store_be32(b.data()+20,32*count);store_be16(b.data()+24,32);b[26]=b[27]=1;store_be32(b.data()+28,0x80006e3c);store_be32(b.data()+32,0x80006e8c);store_be32(b.data()+36,68);return b;}
inline gf::SceneArchivePoolProfile profile(unsigned count=4){const auto b=bytes(count);gf::SceneArchivePoolProfile p;std::string e;if(!p.load({b.data(),b.size()},&e))std::abort();return p;}
inline std::vector<gf::u8> bank(bool fog=false,bool camera_table=true){hsd_record_animation_fixture::Fixture f;if(camera_table)f.ptr(0x18,0xe80);if(fog)f.ptr(0x1c,0xf00);return f.archive();}
}
