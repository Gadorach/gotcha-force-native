#pragma once
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Indices follow the ten r13 count fields cleared by8A28C, NOT host actor slots.
enum class CollisionRegistrationList:u8 {L117C,L177C,L1D7C,L237C,L297C,L2F7C,L357C,L3B7C,L417C,L477C};
struct CollisionRegistrationOwnerFields {
    bool declared=false,present=false;u8 active00=0,kind83=0,disabled1eb=0,disabled276=0;
    u16 suppressed272=0;u32 active76c=0;f32 timer720=0;
};
struct CollisionRegistrationPeerFields {bool declared=false,present=false;u8 kind83=0;f32 timer720=0;};
struct CollisionRegistrationInput {
    bool presence_declared=false,descriptor_declared=false;CollisionRegistrationOwnerFields owner;CollisionRegistrationPeerFields peer;
    u8 descriptor_slot1=0;u16 descriptor_flags2=0;
};
struct CollisionRegistrationPlan {
    u16 lists=0;bool owner_fields_used=false,descriptor_used=false,peer_used=false,owner_timer_used=false,peer_timer_used=false;
    bool contains(CollisionRegistrationList list)const{return (lists&(1u<<unsigned(list)))!=0;}
    bool operator==(const CollisionRegistrationPlan&)const=default;
};
//8A2BC membership candidate ONLY. No registry readiness, actor handle, epoch,
//capacity, collider transform, query result or full callback-list enumeration.
//The source's u8 SLW count is low6 bits;32..63 produce0,64 repeats count0.
inline bool collision_registration_plan(const CollisionRegistrationInput& in,CollisionRegistrationPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("collision registration: ")+why;return false;};
    if(!out||!in.presence_declared)return fail("declared owner presence required");
    CollisionRegistrationPlan plan;auto done=[&](){*out=plan;if(error)error->clear();return true;};
    if(!in.owner.present)return done();
    if(!in.owner.declared)return fail("owner fields required");
    plan.owner_fields_used=true;if(!in.owner.active00)return done();
    if(!in.descriptor_declared)return fail("descriptor required for active owner");
    plan.descriptor_used=true;const auto& owner=in.owner;const u16 enabled=u16(in.descriptor_flags2&u16(~owner.suppressed272));
    auto add=[&](CollisionRegistrationList list,u16 flag){if(enabled&flag)plan.lists|=u16(1u<<unsigned(list));};
    auto timer=[](f32 value){return std::isfinite(value)&&std::abs(value)<=1e7f;};
    if(!owner.kind83){
        const unsigned shift=in.descriptor_slot1&63u;const u32 bit=shift<32?u32(1u<<shift):0u;if(!(owner.active76c&bit))return done();
        plan.owner_timer_used=true;if(!timer(owner.timer720))return fail("owner timer numeric domain");
        if(owner.timer720<=0){add(CollisionRegistrationList::L2F7C,0x40);add(CollisionRegistrationList::L417C,2);}
        add(CollisionRegistrationList::L117C,2);
    }else{
        plan.peer_used=true;if(!in.peer.declared||!in.peer.present)return fail("source secondary owner required");
        bool ready=in.peer.kind83!=0;if(!ready){plan.peer_timer_used=true;if(!timer(in.peer.timer720))return fail("secondary owner timer numeric domain");ready=in.peer.timer720<=0;}
        if(ready)add(CollisionRegistrationList::L2F7C,0x40);
        add(CollisionRegistrationList::L417C,2);
    }
    if(!owner.disabled276)add(CollisionRegistrationList::L477C,1);
    add(CollisionRegistrationList::L3B7C,4);add(CollisionRegistrationList::L357C,8);add(CollisionRegistrationList::L297C,0x10);
    if(!owner.disabled1eb)add(CollisionRegistrationList::L237C,0x20);
    add(CollisionRegistrationList::L1D7C,0x400);
    if(!owner.disabled276)add(CollisionRegistrationList::L177C,0x80);
    return done();
}
} // namespace gf
