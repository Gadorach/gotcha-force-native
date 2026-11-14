#pragma once
#include "tests/unit/battle_preparation_widgets_fixture.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
#include "engine/gameplay/battle_preparation_models.hpp"
#include <functional>
namespace preparation_model_fixture {
inline std::array<gf::u8,64> bytes(const std::vector<gf::u8>& bank){using namespace gf;std::array<u8,64>b{};store_be32(b.data(),0x47465042);store_be32(b.data()+4,1);store_be32(b.data()+8,64);store_be32(b.data()+12,u32(bank.size()));u32 hash=2166136261u;for(u8 v:bank)hash=(hash^v)*16777619u;store_be32(b.data()+16,hash);store_be16(b.data()+20,0xaa0);b[22]=22;b[23]=1;store_be16(b.data()+24,32);store_be16(b.data()+26,32);const std::array<u32,9> values{0x1ee800,0x400,0x80006e3c,0x80006e8c,0xa36800,0x803bb384,68,0,0};for(unsigned i=0;i<values.size();++i)store_be32(b.data()+28+4*i,values[i]);return b;}
struct Files final:gf::IFileSystem {std::vector<gf::u8> data;unsigned reads=0,stats=0,writes=0;bool failed=false,wrong_size=false;std::function<void()> on_read;
    bool read(const std::string&,std::vector<gf::u8>& out)override{++reads;if(on_read)on_read();if(failed)return false;out=data;return true;}
    bool file_size(const std::string&,gf::u64* out)override{++stats;*out=data.size()+unsigned(wrong_size);return true;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{++writes;return false;}bool exists(const std::string&)override{return true;}bool erase(const std::string&)override{++writes;return false;}
};
struct Widgets {std::shared_ptr<gf::SceneDescriptorPool> descriptors=scene_object_fixture::owner(256);std::shared_ptr<gf::SceneObjectPool> objects=std::make_shared<gf::SceneObjectPool>();gf::BattlePreparationWidgets widgets;
    bool open(std::string* e){return objects->begin_fresh(scene_object_fixture::objects(64),descriptors,e)&&widgets.begin(preparation_widget_fixture::profile(),objects,e);}
};
}
