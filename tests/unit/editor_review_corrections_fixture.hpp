#pragma once
#include <map>
#include "engine/gameplay/editor_frontend.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace editor_review_corrections_fixture {
using namespace gf;
struct Save final:ISaveBackend {
    std::map<u32,std::vector<u8>> data;std::array<unsigned,3> writes{};
    bool load(u32 slot,std::vector<u8>& out)override{if(!data.contains(slot))return false;out=data[slot];return true;}
    SaveReadStatus read_slot(u32 slot,std::vector<u8>& out)override{return load(slot,out)?SaveReadStatus::Loaded:SaveReadStatus::Missing;}
    bool store(u32 slot,const u8* bytes,std::size_t size)override{if(slot>2)return false;data[slot]={bytes,bytes+size};++writes[slot];return true;}
    bool erase(u32 slot)override{return data.erase(slot)!=0;}
};
inline bool metadata(const ForceCatalog& catalog,ForceSpecificationCatalog* out){
    std::vector<u8> b(26+catalog.entries().size()*138);store_be32(b.data(),0x47465344);store_be32(b.data()+4,1);store_be32(b.data()+8,catalog.fingerprint());store_be16(b.data()+12,u16(catalog.entries().size()));store_be16(b.data()+14,138);
    for(unsigned i=0;i<catalog.entries().size();++i){const auto at=26+i*138;store_be16(b.data()+at,catalog.entries()[i].model);store_be16(b.data()+at+2,catalog.entries()[i].number);b[at+8]=255;for(unsigned j=8;j<13;++j)b[at+5+j]=255;for(unsigned j=0;j<60;++j)store_be16(b.data()+at+18+2*j,100);}
    std::string error;return out->load({b.data(),b.size()},catalog,&error);
}
struct Fixture {
    ForceCatalog catalog=force_inventory_fixture::catalog();ForceInventory owner;ForceSpecificationCatalog data;ForceSupplement names;Save save;ForceInventoryStore store{save};OptionSettingsStore options{save};CheatSettingsStore cheats{save};MenuNavigationConfig nav;
    bool load(){std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;
        nav.repeat_limit=35;nav.repeat_restart=30;for(unsigned i=0;i<7;++i)nav.neighbor[i]={0,4,u8((i+6)%7),u8((i+1)%7)};
        return metadata(catalog,&data)&&owner.create(catalog)&&names.assign(catalog,{{0x1001,2}},"PART ",variants,digits,{"A","B"},&error);
    }
    bool open(EditorFrontend& frontend,unsigned selected){return frontend.open(owner,names,data,store,options,cheats,nav,{0,1,3,1},selected);}
};
inline GamePadState pad(u32 bits){GamePadState p;p.a=bits&0x100;p.b=bits&0x200;p.x=bits&0x400;p.y=bits&0x800;p.start=bits&0x1000;p.dpad_up=bits&8;p.dpad_down=bits&4;p.dpad_left=bits&1;p.dpad_right=bits&2;p.l=bits&0x40;p.r=bits&0x20;return p;}
}
