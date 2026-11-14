#pragma once
#include <stdexcept>
#include "engine/gameplay/force_inventory.hpp"
namespace force_inventory_fixture {
inline gf::ForceCatalog catalog(){
    std::vector<gf::ForceBorgMetadata> entries;
    for(unsigned i=0;i<3;++i){gf::ForceBorgMetadata x;x.model=gf::u16(0x1001+i);x.number=gf::u16(i);x.tribe=gf::u16(i);x.source_name="OWNED "+std::to_string(i);
        for(unsigned v=0;v<6;++v){x.cost[v]=gf::s16((i==0?100:i==1?250:75)+v*10);x.rarity[v]=gf::s16(v);}
        for(unsigned level=0;level<9;++level)x.experience_thresholds[level]=gf::s16((level+1)*100);
        entries.push_back(std::move(x));
    }
    gf::ForceCatalog out;std::string error;
    if(!out.assign(std::move(entries),{0x1001,0x1002,0x1003},{{0x1001,0x1002,0x1003}},"OWNED FORCE","ABCDEFGHIJKLMNOPQRSTUA","OWNED",21,&error))throw std::runtime_error(error);
    return out;
}
inline gf::ForceBorg borg(unsigned i=0,unsigned variant=0){gf::ForceBorg b;b.model=gf::s16(0x1001+i);b.variant=gf::u8(variant);return b;}
}
