#pragma once
#include "engine/assets/battle_target_query.hpp"
#include "engine/graphics/source_intersection.hpp"
namespace gf {
struct BattleTargetCounterFields {s16 field1c4=0,field1c6=0,counter6b8=0,counter6ba=0;};
//2590C /258C0 return values only. No inferred health/healer/controller policy.
inline bool battle_target_counter_need(const BattleTargetCounterFields& s){return s.field1c4!=s.field1c6||s.counter6b8!=0||s.counter6ba!=0;}
inline bool battle_target_counter_need_strict(const BattleTargetCounterFields& s){return s32(s.field1c4)-s32(s.field1c6)>=100||(s.field1c4!=s.field1c6&&s.field1c6<=50)||s.counter6b8!=0||s.counter6ba!=0;}
struct BattleTargetActorFields {
    bool existence_declared=false,exists=false;
    bool activity_declared=false;u8 active00=0;
    bool kind_declared=false;u8 kind83=0;
    bool normal_fields_declared=false;u8 phase18=0;u32 flags5e0=0;f32 timer720=0;u16 status272=0,model3e8=0;
    bool team_declared=false;u8 team88=0;
    bool position_declared=false;std::array<f32,3> position20{};
};
//25CFC: a nonzero kind83 bypasses normal eligibility, even if active00 is zero.
//25944 has an ADDITIONAL activity/self filter; do not conflate these two callers.
inline bool battle_target_actor_eligible(const BattleTargetActorFields& s,bool* out,std::string* error){
    if(out)*out=false;
    auto fail=[&](){if(error)*error="battle target eligibility: required branch fields unavailable/nonfinite";return false;};
    if(!out||!s.existence_declared)return fail();
    if(!s.exists){if(error)error->clear();return true;}
    if(!s.kind_declared)return fail();
    if(s.kind83){*out=true;if(error)error->clear();return true;}
    if(!s.activity_declared)return fail();
    if(!s.active00){if(error)error->clear();return true;}
    if(!s.normal_fields_declared)return fail();
    if(s.phase18!=1||(s.flags5e0&0x80000000u)){if(error)error->clear();return true;}
    if(!std::isfinite(s.timer720))return fail();
    *out=s.timer720<=0&&!(s.status272&2u)&&s.model3e8!=0xf07;if(error)error->clear();return true;
}
//25A14:0 different team;1/2 same team; every other signed-byte mode returns0.
inline bool battle_target_team_relation(u8 mode30a,u8 owner_team88,u8 peer_team88){return mode30a==0?owner_team88!=peer_team88:(mode30a==1||mode30a==2)&&owner_team88==peer_team88;}
struct DeclaredBattleTargetSlots {bool declared=false;std::array<BattleTargetActorFields,6> slots;};
struct BattleNearestTargetInput {bool declared=false;u8 owner_slot=255,mode30a=0;};
struct BattleNearestTargetResult {
    bool found=false;u8 slot=255,candidate_mask=0;f32 best_squared=0;
    bool operator==(const BattleNearestTargetResult&)const=default;
};
//25944 bounded host-float query over SIX EXPLICIT PHYSICAL SLOT records. There
//is no native registry/actor publication in this declared seam. Missing records
//REFUSE, not computed empty/no-target. Inactive slots need a declared active00.
//BC88 computes squared distance; source <= replaces ties with the LAST slot.
//No sqrt/fitted radius, scene-array reinterpret cast, RNG or target writeback.
inline bool battle_nearest_declared_target(const BattleTargetQueryProfile& p,const DeclaredBattleTargetSlots& pool,const BattleNearestTargetInput& in,BattleNearestTargetResult* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle nearest declared target: ")+why;return false;};
    if(!out||!battle_target_query_profile_valid(p)||!pool.declared||!in.declared||in.owner_slot>=6)return fail("explicit source profile/owner/six-slot snapshot required");
    //Source25944 walks a contiguous six-record pool, not a list of optional
    //pointers. Absent/unknown memory is NOT an inactive or null physical slot.
    for(const auto& s:pool.slots)if(!s.existence_declared||!s.exists||!s.activity_declared)return fail("physical slot activity unavailable");
    BattleNearestTargetResult next;next.best_squared=p.maximum_squared_distance();const auto& owner=pool.slots[in.owner_slot];
    for(unsigned i=0;i<6;++i){const auto& peer=pool.slots[i];if(!peer.active00||i==in.owner_slot)continue;
        if(in.mode30a>2)continue;
        if(!owner.team_declared||!peer.team_declared)return fail("team relation fields unavailable");
        if(!battle_target_team_relation(in.mode30a,owner.team88,peer.team88))continue;
        bool eligible=false;if(!battle_target_actor_eligible(peer,&eligible,error))return false;if(!eligible)continue;
        if(!owner.position_declared||!peer.position_declared||!source_intersection_detail::valid(owner.position20)||!source_intersection_detail::valid(peer.position20))return fail("required positions unavailable/nonfinite/domain");
        const auto delta=source_intersection_detail::sub(owner.position20,peer.position20);const f32 squared=source_intersection_detail::dot(delta,delta);if(!std::isfinite(squared))return fail("distance numeric domain");next.candidate_mask|=u8(1u<<i);
        if(squared<=next.best_squared){next.best_squared=squared;next.slot=u8(i);next.found=true;}
    }
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
