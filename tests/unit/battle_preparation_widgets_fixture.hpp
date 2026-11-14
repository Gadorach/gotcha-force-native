#pragma once
#include "tests/unit/scene_object_pool_fixture.hpp"
#include "engine/gameplay/battle_preparation_widgets.hpp"
namespace preparation_widget_fixture {
inline std::array<gf::u8,144> bytes(){using namespace gf;std::array<u8,144>b{};store_be32(b.data(),0x47464257);store_be32(b.data()+4,1);store_be32(b.data()+8,144);store_be16(b.data()+12,7);store_be16(b.data()+14,16);b[16]=57;b[17]=1;store_be32(b.data()+20,0x80199c0c);store_be16(b.data()+24,372);const std::array<u8,7> counts{1,2,1,3,2,2,1};for(unsigned i=0;i<7;++i){b[32+16*i]=255;b[40+16*i]=counts[i];b[41+16*i]=2;store_be32(b.data()+44+16*i,0x80001000+4*i);}return b;}
inline gf::BattlePreparationWidgetProfile profile(){auto b=bytes();gf::BattlePreparationWidgetProfile p;std::string e;if(!p.load({b.data(),b.size()},&e))std::abort();return p;}
}
