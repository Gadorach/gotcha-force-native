#pragma once
#include "engine/gameplay/type3_cruise_boost.hpp"

namespace gf {
struct NormalDeathIcbmSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0d06&&!variant&&slot<6;}
};
inline bool normal_death_icbm_preselect_refill(NormalDeathIcbmSource source,const CharacterAirResourceState& in,
                                              u32 prior_flags5e0,CharacterAirResourceState* out){
    return source.reviewed()&&type3_preselect_refill(in,prior_flags5e0,out);
}
inline bool normal_death_icbm_boost_request(NormalDeathIcbmSource source,const CharacterMovementInputState& input,
                                           const Type3BoostRequestFields& fields,const CharacterAirResourceState& after_refill,u8* out){
    return source.reviewed()&&type3_boost_request(input,fields,after_refill,out);
}
inline bool normal_death_icbm_boost_debit(NormalDeathIcbmSource source,const CharacterAirResourceState& in,
                                        u32 flags5e0,f32 rate5f4,Type3BoostDebit* out){
    return source.reviewed()&&type3_boost_debit(in,flags5e0,rate5f4,out);
}

enum class IcbmImpactProbeResult:u8 {Unresolved,Clear,Contact};
enum class IcbmImpactQuery:u8 {None,Floor,Scene};
struct IcbmGameImpactInput {
    bool observed_post_class=false;
    u8 role493=0,lifecycle18=0,phase19=0;
    f32 entry_hold720=0;
    u8 contact1d9=0,response1dd=0;
    IcbmImpactProbeResult floor=IcbmImpactProbeResult::Unresolved,scene=IcbmImpactProbeResult::Unresolved;
    bool observed_matrix_translation=false;
    std::array<f32,3> matrix_translation{};
};
struct IcbmGameImpactPlan {
    IcbmImpactQuery next_query=IcbmImpactQuery::None;
    bool request_area_object=false,write_pending_terminal=false;
    u8 owner_slot=0,object_type=0,argument=0;
    s16 hp1c6=0;
    u8 pending663=0;
    u16 mask272=0;
    std::array<f32,3> request_position{};
    bool operator==(const IcbmGameImpactPlan&)const=default;
};
// Exact fictional actor post-class gate. Impact takes observed contact
// sources in order; this neither queries geometry nor allocates an object.
// Terminal writes do not depend on the later area allocation succeeding.
inline bool normal_death_icbm_game_impact(NormalDeathIcbmSource source,const IcbmGameImpactInput& in,IcbmGameImpactPlan* out){
    using P=IcbmImpactProbeResult;
    if(!out||!source.reviewed()||!in.observed_post_class||!std::isfinite(in.entry_hold720)||
       in.floor>P::Contact||in.scene>P::Contact)return false;
    IcbmGameImpactPlan next;
    if(in.role493||in.lifecycle18!=1||std::bit_cast<s8>(in.phase19)<=1||in.entry_hold720>0){
        *out=next;return true;
    }
    if(!in.contact1d9&&!in.response1dd&&in.floor!=P::Contact){
        if(in.floor==P::Unresolved){next.next_query=IcbmImpactQuery::Floor;*out=next;return true;}
        if(in.scene==P::Unresolved)next.next_query=IcbmImpactQuery::Scene;
        if(in.scene!=P::Contact){*out=next;return true;}
    }
    if(!in.observed_matrix_translation)return false;
    for(f32 x:in.matrix_translation)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    next.request_area_object=next.write_pending_terminal=true;
    next.owner_slot=source.slot;next.object_type=73;next.argument=0;
    next.hp1c6=1;next.pending663=1;next.mask272=255;
    next.request_position=in.matrix_translation;
    *out=next;return true;
}

struct IcbmGameDefeatPlan {
    bool request_self_defeat=false;
    u8 slot=0,received1da=0;
    s16 hp1c6=0;
    bool operator==(const IcbmGameDefeatPlan&)const=default;
};
inline bool normal_death_icbm_consume_terminal(NormalDeathIcbmSource source,u8 lifecycle18,u8 pending663,
                                               u8 received1da,bool observed_active_slot_pass,IcbmGameDefeatPlan* out){
    if(!out||!source.reviewed()||!observed_active_slot_pass)return false;
    *out=lifecycle18==1&&pending663?IcbmGameDefeatPlan{true,source.slot,u8(received1da|1u),0}:IcbmGameDefeatPlan{};
    return true;
}
}
