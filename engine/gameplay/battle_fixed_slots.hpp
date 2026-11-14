#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>
namespace gf {
//536B0/537A8 seed fields, not full actor bodies, program players or world frames.
//All six physical pointers exist even when model16 isFFFF (inactive).
struct BattleFixedSlotInput {u16 model16=0xffff;std::optional<u8> variant_a0,member84;};
struct BattleFixedSlot {
    u8 active00=0,kind83=0,slot3e4=0,mask3e5=0,resource3e7=0;
    u16 model3e8=0,model3ea=0,render94=0;
    u8 variant3ec=0,variant3ed=0,member492=0,field3f2=0,field486=0,render96=0,resource97=0;
    bool self8c=false,self90=false; //native SAME-slot relationships, never guest pointer bits
    bool operator==(const BattleFixedSlot&)const=default;
};
enum class BattleFixedSlotWriteKind:u8 {PublishPhysical,Byte,Half,Clear,SelfReference};
struct BattleFixedSlotWrite {BattleFixedSlotWriteKind kind;u8 slot;u16 offset,extent;u32 value;bool operator==(const BattleFixedSlotWrite&)const=default;};
struct BattleFixedSlotPlan {std::array<BattleFixedSlot,6> slots{};std::vector<BattleFixedSlotWrite> writes;};
//Declared numeric kernel. Native admission separately requires the current
//normal-party producer and exact owned physical bank; this plan grants neither.
inline bool battle_fixed_slot_seed(const std::array<BattleFixedSlotInput,6>& input,BattleFixedSlotPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="fixed actor slots: reached model/variant/member domain unknown";return false;};
    if(!out)return fail();
    for(const auto& in:input)if(in.model16!=0xffff&&(!in.variant_a0||!in.member84||*in.variant_a0>=6||*in.member84>=30))return fail();
    BattleFixedSlotPlan plan;using K=BattleFixedSlotWriteKind;
    for(unsigned i=0;i<6;++i){auto& s=plan.slots[i];const auto& in=input[i];auto put=[&](K k,u16 offset,u16 extent,u32 value){plan.writes.push_back({k,u8(i),offset,extent,value});};
        put(K::PublishPhysical,0,4,i);put(K::Byte,0x3e4,1,i);put(K::Clear,0,0x1e00,0);
        s.slot3e4=s.resource3e7=u8(i);s.mask3e5=u8(1u<<i);put(K::Byte,0x83,1,0);put(K::Byte,0x3e4,1,i);put(K::Byte,0x3e5,1,s.mask3e5);put(K::Byte,0x3e7,1,i);
        if(in.model16==0xffff)continue;
        s.active00=1;s.model3ea=s.model3e8=s.render94=in.model16;s.variant3ed=s.variant3ec=*in.variant_a0;s.member492=*in.member84;s.self8c=s.self90=true;s.render96=s.slot3e4;s.resource97=s.resource3e7;
        put(K::Byte,0,1,1);put(K::Half,0x3ea,2,s.model3ea);put(K::Half,0x3e8,2,s.model3e8);put(K::Byte,0x3ed,1,s.variant3ed);put(K::Byte,0x3ec,1,s.variant3ec);put(K::Byte,0x492,1,s.member492);put(K::Byte,0x3f2,1,0);put(K::Byte,0x486,1,0);put(K::SelfReference,0x8c,4,i);put(K::SelfReference,0x90,4,i);put(K::Half,0x94,2,s.render94);put(K::Byte,0x96,1,s.render96);put(K::Byte,0x97,1,s.resource97);
    }
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
