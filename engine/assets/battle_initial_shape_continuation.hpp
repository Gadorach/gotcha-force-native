#pragma once
#include <optional>
#include "engine/assets/battle_initial_shape_child.hpp"
namespace gf {
struct BattleInitialShapeOwnerRow {u8 slot=0;u16 model=0;u32 hash=0;std::array<u8,2> bones{16,19};};
//The common light recipe does not authorize an actor. These sealed source rows
//bind the additional type0 shapes to their actual parent slots/model banks.
class BattleInitialShapeContinuationProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;light_={};auto fail=[&](){if(error)*error="initial shape continuation: exact source slot/model/bone rows required";return false;};
        if(!b.contains(0,212)||b.size!=212||b.be32(0)!=0x47465348||b.be32(4)!=1||b.be32(8)!=212||b.be32(12))return fail();
        if(!light_.load({b.data+16,116},error))return false;
        const std::array<u32,20> rows{0,0x070b,0xbdb1d4f4,16,19,1,0x000a,0xeab3632a,16,19,2,0x0105,0x4fb1157d,16,19,3,0x0615,0xe2b4439a,16,19};
        for(unsigned i=0;i<rows.size();++i)if(b.be32(132+4*i)!=rows[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const BattleInitialShapeChildProfile* light_recipe()const{return ready_?&light_:nullptr;}
    std::optional<BattleInitialShapeOwnerRow> owner(unsigned slot)const{
        if(!ready_||slot>=4)return {};
        constexpr std::array<u16,4> models{0x70b,10,0x105,0x615};
        constexpr std::array<u32,4> hashes{0xbdb1d4f4,0xeab3632a,0x4fb1157d,0xe2b4439a};
        return BattleInitialShapeOwnerRow{u8(slot),models[slot],hashes[slot],{16,19}};
    }
private:bool ready_=false;BattleInitialShapeChildProfile light_;
};
}
